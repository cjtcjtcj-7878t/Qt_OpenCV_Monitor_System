#include "monitorworker.h"
#include <QDebug>

#include <QDir>
#include <QCoreApplication>
#include <QStringList>
#include <cmath>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QTimer>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace
{
QImage matToImage(const cv::Mat &frame)
{
    //将opencv中的Mat类转换为Qt中的Qimage类进行显示在Qt界面
    if(frame.empty()){
        return QImage();
    }

    cv::Mat converted;
    if(frame.channels()==1){
        return QImage(frame.data,frame.cols,frame.rows,static_cast<int>(frame.step),QImage::Format_Grayscale8).copy();
    }
    if(frame.channels()==3){
        cv::cvtColor(frame,converted,cv::COLOR_BGR2RGB);
        return QImage(converted.data,converted.cols,converted.rows,static_cast<int>(converted.step),QImage::Format_RGB888).copy();
    }
    if(frame.channels()==4){
        cv::cvtColor(frame,converted,cv::COLOR_BGRA2RGBA);
        return QImage(converted.data,converted.cols,converted.rows,static_cast<int>(converted.step),QImage::Format_RGBA8888).copy();
    }
    return QImage();
}
}

monitorworker::monitorworker(QObject *parent)
    : QObject(parent)
{
}

monitorworker::~monitorworker()
{
    if(capture_timer){
        capture_timer->stop();
    }
    for(int index=0;index<channel_count;++index){
        closeChannel(index);
    }
}

void monitorworker::initialize()
{
    if(capture_timer){
        return;
    }

    capture_timer=new QTimer(this);
    capture_timer->setTimerType(Qt::PreciseTimer);
    capture_timer->setInterval(10);
    connect(capture_timer,&QTimer::timeout,this,&monitorworker::readNextFrames);
}

void monitorworker::shutdown()
{
    closeAllCameras(0);
    QThread::currentThread()->quit();
}

void monitorworker::openCamera(int viewIndex,int deviceIndex,int cameraId,int intervalSeconds,const QString &newVideoDirectory,const QString &newImageDirectory)
{
    if(!isValidIndex(viewIndex)){
        return;
    }

    if(!capture_timer){
        initialize();
    }
    closeChannel(viewIndex);

    if(newVideoDirectory.trimmed().isEmpty()||newImageDirectory.trimmed().isEmpty()){
        emit errorOccurred(viewIndex,"录像或截图保存路径为空，请先在设置中配置!");
        updateCaptureTimer();
        return;
    }

    QDir videoDir(newVideoDirectory.trimmed());
    QDir imageDir(newImageDirectory.trimmed());
    if((!videoDir.exists()&&!videoDir.mkpath("."))
        ||(!imageDir.exists()&&!imageDir.mkpath("."))){
        emit errorOccurred(viewIndex,"无法创建录像或截图保存目录!");
        updateCaptureTimer();
        return;
    }

    channel_session &channel=channels[viewIndex];
    QString errorMessage;
    if(!channel.camera_capture.openCamera(deviceIndex,&errorMessage)){
        emit errorOccurred(viewIndex,errorMessage);
        updateCaptureTimer();
        return;
    }

    channel.camera_id=cameraId;
    channel.recording_interval_seconds=qMax(1,intervalSeconds);
    channel.frame_interval_ms=qMax(
        1,static_cast<int>(1000.0/channel.camera_capture.fps()));
    channel.video_directory=videoDir.absolutePath();
    channel.image_directory=imageDir.absolutePath();
    channel.current_frame.release();
    channel.frame_timer.start();
    updateCaptureTimer();
    emit cameraStateChanged(viewIndex,true);
}

// virtualIndex 是虚拟设备的顺序编号，与真实摄像头的设备编号无关。
void monitorworker::openVirtualCamera(int viewIndex, int virtualIndex)
{
    if(!isValidIndex(viewIndex) || virtualIndex < 0){
        return;
    }
    initialize();
    closeChannel(viewIndex);

    // 支持 exe/data/Videos 和 bin/../data/Videos，不依赖工作目录。
    const QDir appDir(QCoreApplication::applicationDirPath());
    QDir sourceDir(appDir.filePath("data/Videos"));
    if(!sourceDir.exists()){
        sourceDir.setPath(appDir.filePath("../data/Videos"));
    }
    QFileInfoList videos;
    const QStringList extensions={"mp4","avi","mkv","mov","wmv","m4v","webm"};
    const QFileInfoList entries=sourceDir.entryInfoList(QDir::Files | QDir::Readable,
                                                          QDir::Name | QDir::IgnoreCase);
    for(const QFileInfo &entry : entries){
        if(extensions.contains(entry.suffix().toLower())){
            videos.append(entry);
        }
    }
    if(videos.isEmpty()){
        emit errorOccurred(viewIndex,QString("未找到示例视频，请检查：%1 或 %2")
                                          .arg(appDir.filePath("data/Videos"),
                                               appDir.filePath("../data/Videos")));
        updateCaptureTimer();
        return;
    }

    channel_session &channel=channels[viewIndex];
    const QString path=videos.at(virtualIndex % videos.size()).absoluteFilePath();
    try {
        if(!channel.virtual_capture.open(path.toUtf8().constData())){
            emit errorOccurred(viewIndex,QString("无法打开示例视频：%1").arg(path));
            updateCaptureTimer();
            return;
        }
        channel.virtual_video_path=path;
        double fps=channel.virtual_capture.get(cv::CAP_PROP_FPS);
        if(!std::isfinite(fps) || fps < 1.0 || fps > 240.0){
            fps=25.0;
        }
        channel.frame_interval_ms=qMax(1,static_cast<int>(std::round(1000.0/fps)));
        channel.frame_timer.start();
        emit cameraStateChanged(viewIndex,true);
    } catch(const cv::Exception &error) {
        closeChannel(viewIndex);
        emit errorOccurred(viewIndex,QString("示例视频打开失败：%1；%2")
                                          .arg(path,QString::fromUtf8(error.what())));
    }
    updateCaptureTimer();
}

void monitorworker::closeCamera(int viewIndex)
{
    if(!isValidIndex(viewIndex)){
        return;
    }
    closeChannel(viewIndex);
    updateCaptureTimer();
}

void monitorworker::closeAllCameras(quint64 requestId)
{
    if(capture_timer){
        capture_timer->stop();
    }
    for(int index=0;index<channel_count;++index){
        closeChannel(index);
    }
    emit closeCompleted(requestId);
}

void monitorworker::flushSegments()
{
    // 切换到回放页面前先结束当前分段，短于 Time_interval 的视频也能立即落盘。
    for(int index=0;index<channel_count;++index){
        finishSegment(index);
    }
    emit segmentsFlushed();
}

void monitorworker::setDetectionMode(bool enabled)
{
    if(detection_mode==enabled){
        return;
    }

    if(enabled){
        // 切换到移动侦测前先结束普通录像分段，两个表不会记录同一段视频。
        for(int index=0;index<channel_count;++index){
            finishSegment(index);
        }
    }
    detection_mode=enabled;
}

void monitorworker::takeSnapshot(int viewIndex,const QString &newImageDirectory)
{
    if(viewIndex<0){
        for(int index=0;index<channel_count;++index){
            if(channels[index].camera_capture.isOpened()){
                takeSnapshot(index,newImageDirectory);
            }
        }
        return;
    }
    if(!isValidIndex(viewIndex)){
        return;
    }

    channel_session &channel=channels[viewIndex];
    if(!channel.camera_capture.isOpened()||channel.current_frame.empty()){
        emit errorOccurred(viewIndex,"当前没有可截取的监控画面!");
        return;
    }

    const QString directory = newImageDirectory.trimmed();

    if (directory.isEmpty() || !QDir::isAbsolutePath(directory)) {
        emit errorOccurred(viewIndex,QString("图片保存路径必须是非空的绝对路径：\n%1").arg(directory));
        return;
    }

    QDir saveDirectory(directory);

    if (!saveDirectory.exists() && !saveDirectory.mkpath(".")) {
        emit errorOccurred(viewIndex,QString("截图目录创建失败：\n%1").arg(saveDirectory.absolutePath()));
        return;
    }

    const QString timestamp=QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    const QString prefix=detection_mode?"detect_camera":"camera";
    QString imagePath;
    for(int index=0;index<1000;++index){
        const QString suffix=index==0?QString():QString("_%1").arg(index);
        imagePath=saveDirectory.filePath(
            QString("%1_%2_snapshot_%3%4.png")
                .arg(prefix).arg(channel.camera_id).arg(timestamp,suffix));
        if(!QFile::exists(imagePath)){
            break;
        }
    }

    try
    {
        std::vector<uchar> encoded;

        if (!cv::imencode(".png", channel.current_frame, encoded)) {
            emit errorOccurred(viewIndex, "截图 PNG 编码失败！");
            return;
        }

        QSaveFile file(imagePath);

        if (!file.open(QIODevice::WriteOnly)) {
            emit errorOccurred(viewIndex,QString("无法创建截图文件：\n%1\n原因：%2").arg(imagePath, file.errorString()));
            return;
        }

        const qint64 size = static_cast<qint64>(encoded.size());

        if (file.write(reinterpret_cast<const char *>(encoded.data()),size) != size) {
            const QString error = file.errorString();
            file.cancelWriting();

            emit errorOccurred(viewIndex,QString("截图写入失败：\n%1\n原因：%2").arg(imagePath, error));
            return;
        }

        if (!file.commit()) {
            emit errorOccurred(viewIndex,QString("截图保存失败：\n%1\n原因：%2").arg(imagePath, file.errorString()));
            return;
        }

        qInfo().noquote() << "截图已保存：" << imagePath;

    }
    catch (const cv::Exception &exception) {
        emit errorOccurred(viewIndex,QString("截图编码异常：%1").arg(QString::fromUtf8(exception.what())));
        return;
    }

    // 移动侦测模式下视频路径由 detectworker 管理，因此这里允许路径为空。
    emit snapshotFinished(channel.camera_id,imagePath,channel.current_video_path);
}

void monitorworker::readNextFrames()
{
    for(int viewIndex=0;viewIndex<channel_count;++viewIndex){
        channel_session &channel=channels[viewIndex];
        if(!channel.camera_capture.isOpened() && !channel.virtual_capture.isOpened()){
            continue;
        }
        if(channel.frame_timer.isValid()
            &&channel.frame_timer.elapsed()<channel.frame_interval_ms){
            continue;
        }
        channel.frame_timer.restart();

        cv::Mat frame;
        if(channel.virtual_capture.isOpened()){
            try {
                bool readOk=channel.virtual_capture.read(frame) && !frame.empty();
                if(!readOk){
                    // 文件到尾后重置；不支持 seek 的解码器尝试重新打开一次。
                    channel.virtual_capture.set(cv::CAP_PROP_POS_FRAMES,0);
                    readOk=channel.virtual_capture.read(frame) && !frame.empty();
                    if(!readOk){
                        channel.virtual_capture.release();
                        readOk=channel.virtual_capture.open(
                                     channel.virtual_video_path.toUtf8().constData())
                                 && channel.virtual_capture.read(frame) && !frame.empty();
                    }
                }
                if(!readOk){
                    closeChannel(viewIndex);
                    emit errorOccurred(viewIndex,"示例视频无法读取，请检查文件及解码器");
                    continue;
                }
                channel.current_frame=frame;
                if(detection_mode){
                    // 负数 cameraId 表示只做检测预览，不建立录像/截图数据库记录。
                    emit frameForDetection(viewIndex,frame,-viewIndex-1,
                                           1000.0/channel.frame_interval_ms,
                                           30,QString(),QString());
                }else{
                    emitCurrentFrame(viewIndex,frame);
                }
            } catch(const cv::Exception &error) {
                closeChannel(viewIndex);
                emit errorOccurred(viewIndex,QString("示例视频解码失败：%1")
                                                  .arg(QString::fromUtf8(error.what())));
            }
            // 示例视频可参与检测画框，但不重复录像。
            continue;
        }
        if(!channel.camera_capture.readFrame(frame)){
            emit errorOccurred(viewIndex,"摄像头读取画面失败!");
            closeChannel(viewIndex);
            continue;
        }
        channel.current_frame=frame;

        if(detection_mode){
            // 移动侦测线程接管录像和画框；这里仅传递一份引用计数的 Mat，避免额外拷贝。
            emit frameForDetection(viewIndex,
                                   frame,
                                   channel.camera_id,
                                   channel.camera_capture.fps(),
                                   channel.recording_interval_seconds,
                                   channel.video_directory,
                                   channel.image_directory);
        }else{
            if(!channel.camera_capture.isRecording()){
                if(!startSegment(viewIndex,frame.size())){
                    emit errorOccurred(viewIndex,"录像文件创建失败，已停止摄像头!");
                    closeChannel(viewIndex);
                    continue;
                }
            }else if(channel.segment_timer.isValid()
                       &&channel.segment_timer.elapsed()
                              >=qint64(channel.recording_interval_seconds)*1000){
                finishSegment(viewIndex);
                if(!startSegment(viewIndex,frame.size())){
                    emit errorOccurred(viewIndex,"下一段录像文件创建失败，已停止摄像头!");
                    closeChannel(viewIndex);
                    continue;
                }
            }

            try {
                const bool ok = channel.timeline.append(
                    frame, channel.segment_timer.elapsed(),
                    [&channel](const cv::Mat &sample) {
                        return channel.camera_capture.writeFrame(sample);
                    });
                if (!ok) emit errorOccurred(viewIndex, "录像帧写入失败!");
            } catch (const cv::Exception &error) {
                emit errorOccurred(viewIndex, QString::fromUtf8(error.what()));
                closeChannel(viewIndex);
                continue;
            }
            emitCurrentFrame(viewIndex,frame);
        }
    }
    updateCaptureTimer();
}

bool monitorworker::isValidIndex(int viewIndex) const
{
    return viewIndex>=0&&viewIndex<channel_count;
}

bool monitorworker::hasOpenedCamera() const
{
    for(const channel_session &channel : channels){
        if(channel.camera_capture.isOpened() || channel.virtual_capture.isOpened()){
            return true;
        }
    }
    return false;
}

void monitorworker::updateCaptureTimer()
{
    if(!capture_timer){
        return;
    }
    if(hasOpenedCamera()){
        if(!capture_timer->isActive()){
            capture_timer->start();
        }
    }else{
        capture_timer->stop();
    }
}

void monitorworker::closeChannel(int viewIndex)
{
    channel_session &channel=channels[viewIndex];
    const bool hadCamera=channel.camera_capture.isOpened()|| channel.virtual_capture.isOpened()|| !channel.virtual_video_path.isEmpty();
    finishSegment(viewIndex);
    channel.camera_capture.closeCamera();
    channel.virtual_capture.release();
    channel.virtual_video_path.clear();
    channel.current_frame.release();
    channel.current_video_path.clear();
    channel.current_segment_start=QDateTime();
    channel.segment_timer.invalidate();
    channel.frame_timer.invalidate();
    channel.camera_id=0;

    if(hadCamera){
        emit cameraStateChanged(viewIndex,false);
    }
}

bool monitorworker::startSegment(int viewIndex,const cv::Size &frameSize)
{
    channel_session &channel=channels[viewIndex];
    const QDateTime startTime=QDateTime::currentDateTime();
    const QString timestamp=startTime.toString("yyyyMMdd_hhmmss_zzz");
    QString path;
    for(int index=0;index<1000;++index){
        const QString suffix=index==0?QString():QString("_%1").arg(index);
        path=QDir(channel.video_directory).filePath(QString("camera_%1_%2%3.avi").arg(channel.camera_id).arg(timestamp,suffix));
        if(!QFile::exists(path)){
            break;
        }
    }

    QString errorMessage;
    if(!channel.camera_capture.startRecording(path,
                                               channel.camera_capture.fps(),
                                               frameSize,
                                               &errorMessage)){
        emit errorOccurred(viewIndex,errorMessage);
        return false;
    }

    channel.current_video_path=path;
    channel.current_segment_start=startTime;
    channel.timeline.reset(channel.camera_capture.fps(),
                           qint64(channel.recording_interval_seconds) * 1000);
    channel.segment_timer.restart();
    emit recordingStarted(channel.camera_id,path,startTime);
    return true;
}

void monitorworker::finishSegment(int viewIndex)
{
    channel_session &channel=channels[viewIndex];
    if(!channel.camera_capture.isRecording()||channel.current_video_path.isEmpty()){
        return;
    }

    // 在封口前冻结本段时长，避免补帧/编码本身耗时继续增长目标。
    const qint64 elapsed = channel.segment_timer.elapsed();
    try {
        if (!channel.timeline.padTo(elapsed, [&channel](const cv::Mat &sample) {
                return channel.camera_capture.writeFrame(sample);
            }))
            emit errorOccurred(viewIndex, "普通录像补帧失败，文件可能不完整");
    } catch (const cv::Exception &error) {
        emit errorOccurred(viewIndex, QString("普通录像封口失败：%1").arg(QString::fromUtf8(error.what())));
    }
    channel.camera_capture.stopRecording();
    qInfo() << "普通录像：经过(ms)=" << elapsed
            << "帧数=" << channel.timeline.frameCount()
            << "文件时长(ms)=" << channel.timeline.durationMs();
    const QString path=channel.current_video_path;
    const qint64 fileSize=QFileInfo(path).size();
    const QDateTime endTime=channel.current_segment_start.addMSecs(channel.timeline.durationMs());
    channel.current_video_path.clear();
    channel.current_segment_start=QDateTime();
    channel.segment_timer.invalidate();
    emit recordingFinished(channel.camera_id,path,fileSize,endTime);
}

void monitorworker::emitCurrentFrame(int viewIndex,const cv::Mat &frame)
{
    const QImage image=matToImage(frame);
    if(!image.isNull()){
        emit frameReady(viewIndex,image);
    }
}
