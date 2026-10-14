#include "detectworker.h"
#include <QDebug>
#include <QSaveFile>
#include <QUuid>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <cmath>
#include <exception>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

detectworker::detectworker(QObject *parent)
    : QObject(parent)
{
    for(auto &pending : frame_pending){
        pending.store(false,std::memory_order_relaxed);
    }
}

detectworker::~detectworker()
{
    setActive(false);
}

void detectworker::requestFrameAcceptance(bool accepted) noexcept
{
    accepting_frames.store(accepted,std::memory_order_release);
}

bool detectworker::tryReserveFrame(int viewIndex) noexcept
{
    if(!isValidIndex(viewIndex)
        ||!accepting_frames.load(std::memory_order_acquire)){
        return false;
    }

    bool expected=false;
    // CAS 成功表示该通道当前没有排队帧；失败就丢弃这一帧。
    return frame_pending[viewIndex].compare_exchange_strong(
        expected,true,std::memory_order_acq_rel,std::memory_order_relaxed);
}

void detectworker::initialize()
{
    if(initialized){
        return;
    }

    // 线程先完成初始化；模型等真正切换到移动侦测时再加载，减少普通模式内存占用。
    initialized=true;
}

void detectworker::loadDetector()
{
    if(detector_load_attempted){
        return;
    }
    detector_load_attempted=true;

    // 模型只在移动侦测线程中加载，避免启动界面时占用主线程。
    const QDir appDir(QCoreApplication::applicationDirPath());
    QString modelPath=appDir.filePath("data/model/yolov8n.onnx");
    if(!QFileInfo::exists(modelPath)){
        modelPath=appDir.filePath("../data/model/yolov8n.onnx");
    }
    if(QFileInfo::exists(modelPath)){
        try{
            detector=std::make_unique<Detect>(modelPath.toStdString());
            detector->model_input_size=cv::Size(640,640);
        }catch(const cv::Exception &exception){
            qWarning()<<"加载移动侦测模型失败:"<<exception.what();
            detector.reset();
            emit errorOccurred(-1,QString("加载检测模型失败：%1").arg(QString::fromUtf8(exception.what())));
        }
        catch(const std::exception &exception){
            qWarning()<<"加载移动侦测模型失败:"<<exception.what();
            detector.reset();
            emit errorOccurred(-1,QString("加载检测模型失败：%1").arg(QString::fromUtf8(exception.what())));
        }
    }else{
        // 模型缺失时仍然可以录制 detect_t 视频，只是不绘制检测框。
        qWarning()<<"未找到移动侦测模型:"<<modelPath;
        emit errorOccurred(-1,QString("未找到 yolov8n.onnx，无法画框。请检查：%1 或 %2").arg(appDir.filePath("data/model/yolov8n.onnx"),appDir.filePath("../data/model/yolov8n.onnx")));
    }
}

void detectworker::shutdown()
{
    setActive(false);
    QThread::currentThread()->quit();
}

void detectworker::setActive(bool newActive)
{
    if(active==newActive){
        return;
    }
    active=newActive;
    accepting_frames.store(active,std::memory_order_release);
    if(active){
        for (int index = 0; index < channel_count; ++index) {
            sessions[index].startup_snapshot_attempts = 0;
            sessions[index].frame_count = 0;
            sessions[index].detection_event_active = false;
            detection_results[index].clear();
        }
        if(!initialized){
            initialize();
        }
        loadDetector();
        return;
    }

    if(!active){
        // 切回普通模式时先封闭当前移动侦测视频，保证文件和数据库时间完整。
        for(int index=0;index<channel_count;++index){
            finishSegment(index);
        }
        // 普通监控不需要目标检测模型，释放它可以降低持续运行时的内存占用。
        detector.reset();
        detector_load_attempted=false;
        for(int index=0;index<channel_count;++index){
            detection_results[index].clear();
            sessions[index].detection_event_active=false;
        }
    }
}

void detectworker::flushSegments()
{
    // 结束当前移动侦测分段，确保短视频也能被 OpenCV 正确关闭和播放。
    for(int index=0;index<channel_count;++index){
        finishSegment(index);
    }
    emit segmentsFlushed();
}

void detectworker::processFrame(int viewIndex,
                                const cv::Mat &inputFrame,
                                int cameraId,
                                double cameraFps,
                                int intervalSeconds,
                                const QString &videoDirectory,
                                const QString &imageDirectory)
{
    if(!isValidIndex(viewIndex)){
        return;
    }
    // 取出队列中的这一帧后立刻释放占位，允许该通道再排入一帧。
    frame_pending[viewIndex].store(false,std::memory_order_release);
    if(!active||!accepting_frames.load(std::memory_order_acquire)
        ||inputFrame.empty()){
        return;
    }

    // 虚拟视频使用负数标记，复用检测与画框，但不写文件/数据库。
    const bool previewOnly=cameraId<0;
    detect_session &session=sessions[viewIndex];
    if(session.camera_id!=cameraId){
        finishSegment(viewIndex);
        detection_results[viewIndex].clear();
        session.detection_event_active=false;
        session.frame_count=0;
    }
    const QString directory=QDir(videoDirectory.trimmed()).absolutePath();

    // 摄像头配置改变时，先结束旧分段，再用新配置建立分段。
    if(session.writer.isOpened()
        &&(session.camera_id!=cameraId||session.video_directory!=directory)){
        finishSegment(viewIndex);
    }
    session.camera_id=cameraId;
    session.fps=(cameraFps>0.0&&std::isfinite(cameraFps))?cameraFps:25.0;
    session.recording_interval_seconds=qMax(1,intervalSeconds);
    session.video_directory=directory;
    // 保留空值，不能把空路径偷偷转换为工作目录。
    session.image_directory = imageDirectory.trimmed();

    cv::Mat outputFrame=inputFrame.clone();
    if(detector){
        try{
            // 每5帧做一次推理，其余帧沿用上一轮结果，降低CPU和内存压力。
            if(session.frame_count%5==0){
                detector->org_Size=outputFrame.size();
                const cv::Mat modelFrame=detector->letterbox(
                    outputFrame,
                    detector->model_input_size.width,
                    detector->model_input_size.height,
                    cv::Scalar());
                detection_results[viewIndex].clear();
                detector->detect(modelFrame,detection_results[viewIndex]);

                // 自动截图由开启侦测后的三帧计数控制，不再由目标出现触发。
            }
            detector->draw(outputFrame,detection_results[viewIndex]);
        }catch(const cv::Exception &exception){
            emit errorOccurred(viewIndex,
                               QString("移动侦测处理失败：%1").arg(exception.what()));
            detector.reset();
            detection_results[viewIndex].clear();
            session.detection_event_active=false;
        }
    }
    // 只需保留推理周期，避免虚拟视频长时间循环导致计数溢出。
    session.frame_count=(session.frame_count+1)%5;

    if(previewOnly){
        const QImage image=matToImage(outputFrame);
        if(!image.isNull()){
            emit frameReady(viewIndex,image);
        }
        return;
    }

    if (session.writer.isOpened() && session.segment_timer.isValid()
        && session.segment_timer.elapsed() >= qint64(session.recording_interval_seconds)*1000) {
        finishSegment(viewIndex);
    }

    if(!session.writer.isOpened()&&!startSegment(viewIndex,outputFrame.size())){
        emit errorOccurred(viewIndex,"移动侦测录像文件创建失败!");
        return;
    }
    if(!session.writer.isOpened()){
        return;
    }
    try {
        session.timeline.append(outputFrame, session.segment_timer.elapsed(),
                                [&session](const cv::Mat &sample) {
                                    session.writer.write(sample);
                                    return true;
                                });
    } catch (const cv::Exception &error) {
        emit errorOccurred(viewIndex, QString("侦测录像写入失败：%1")
                                          .arg(QString::fromUtf8(error.what())));
        finishSegment(viewIndex);
        return;
    }

    // 此时录像已建立且带框帧已经写入，截图可以准确关联当前 detect_t 记录。
    if (session.startup_snapshot_attempts < 3) {
        // 每帧只尝试一次；失败报告具体原因，不无限重试或多截后续帧。
        ++session.startup_snapshot_attempts;
        saveAutomaticSnapshot(viewIndex, outputFrame);
    }

    const QImage image=matToImage(outputFrame);
    if(!image.isNull()){
        emit frameReady(viewIndex,image);
    }
}

bool detectworker::isValidIndex(int viewIndex) const
{
    return viewIndex>=0&&viewIndex<channel_count;
}

bool detectworker::startSegment(int viewIndex,const cv::Size &frameSize)
{
    detect_session &session=sessions[viewIndex];
    QDir directory(session.video_directory);
    if(session.video_directory.trimmed().isEmpty()
        ||(!directory.exists()&&!directory.mkpath("."))){
        return false;
    }

    const QDateTime startTime=QDateTime::currentDateTime();
    const QString timestamp=startTime.toString("yyyyMMdd_hhmmss_zzz");
    QString path;
    for(int index=0;index<1000;++index){
        const QString suffix=index==0?QString():QString("_%1").arg(index);
        path=directory.filePath(QString("detect_camera_%1_%2%3.avi").arg(session.camera_id).arg(timestamp,suffix));
        if(!QFile::exists(path)){
            break;
        }
    }

    if(!session.writer.open(path.toUtf8().constData(),
                             cv::VideoWriter::fourcc('M','J','P','G'),
                             session.fps,
                             frameSize,
                             true)){
        return false;
    }
    session.current_video_path=path;
    session.segment_start=startTime;
    session.timeline.reset(session.fps, qint64(session.recording_interval_seconds)*1000);
    session.segment_timer.restart();
    session.frame_count=0;
    emit recordingStarted(session.camera_id,path,startTime);
    return true;
}

void detectworker::finishSegment(int viewIndex)
{
    if(!isValidIndex(viewIndex)){
        return;
    }
    detect_session &session=sessions[viewIndex];
    if(!session.writer.isOpened()||session.current_video_path.isEmpty()){
        return;
    }
    const qint64 elapsed = session.segment_timer.elapsed();
    try {
        session.timeline.padTo(elapsed, [&session](const cv::Mat &sample) {
            session.writer.write(sample);
            return true;
        });
    } catch (const cv::Exception &error) {
        emit errorOccurred(viewIndex, QString("侦测录像封口失败：%1").arg(QString::fromUtf8(error.what())));
    }
    session.writer.release();
    qInfo() << "侦测录像：经过(ms)=" << elapsed
            << "帧数=" << session.timeline.frameCount()
            << "文件时长(ms)=" << session.timeline.durationMs();
    const QString path=session.current_video_path;
    const qint64 fileSize=QFileInfo(path).size();
    const QDateTime endTime=session.segment_start.addMSecs(session.timeline.durationMs());
    const int cameraId=session.camera_id;
    session.current_video_path.clear();
    session.segment_start=QDateTime();
    session.segment_timer.invalidate();
    emit recordingFinished(cameraId,path,fileSize,endTime);
}

void detectworker::saveAutomaticSnapshot(int viewIndex, const cv::Mat &frame)
{
    if (!isValidIndex(viewIndex) || frame.empty()) return;
    detect_session &session = sessions[viewIndex];
    const QString root = session.image_directory.trimmed();
    if (root.isEmpty() || !QDir::isAbsolutePath(root)) {
        emit errorOccurred(viewIndex, "自动截图失败：请在设置中选择有效的图片绝对路径！");
        return;
    }
    QDir directory(root);
    if (!directory.exists() && !directory.mkpath(".")) {
        emit errorOccurred(viewIndex, QString("无法创建自动截图目录：\n%1").arg(root));
        return;
    }
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    const QString imagePath = directory.absoluteFilePath(
        QString("detect_camera_%1_event_%2_start%3_%4.png")
            .arg(session.camera_id).arg(timestamp)
            .arg(session.startup_snapshot_attempts)
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    try {
        std::vector<uchar> encoded;
        if (!cv::imencode(".png", frame, encoded)) {
            emit errorOccurred(viewIndex, "自动截图 PNG 编码失败！");
            return;
        }
        QSaveFile file(imagePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit errorOccurred(viewIndex, QString("无法创建自动截图：\n%1\n%2").arg(imagePath, file.errorString()));
            return;
        }
        const qint64 size = static_cast<qint64>(encoded.size());
        if (file.write(reinterpret_cast<const char *>(encoded.data()), size) != size) {
            const QString error = file.errorString();
            file.cancelWriting();
            emit errorOccurred(viewIndex, QString("自动截图写入失败：\n%1\n%2").arg(imagePath, error));
            return;
        }
        if (!file.commit()) {
            emit errorOccurred(viewIndex, QString("自动截图提交失败：\n%1\n%2").arg(imagePath, file.errorString()));
            return;
        }
    } catch (const cv::Exception &error) {
        emit errorOccurred(viewIndex, QString("自动截图编码异常：%1").arg(QString::fromUtf8(error.what())));
        return;
    }
    qInfo().noquote() << "开启侦测自动截图" << session.startup_snapshot_attempts<< "/3：" << imagePath;
    emit automaticSnapshotFinished(session.camera_id, imagePath,session.current_video_path);
}

QImage detectworker::matToImage(const cv::Mat &frame) const
{
    if(frame.empty()){
        return QImage();
    }
    cv::Mat converted;
    if(frame.channels()==3){
        cv::cvtColor(frame,converted,cv::COLOR_BGR2RGB);
        return QImage(converted.data,converted.cols,converted.rows,static_cast<int>(converted.step),QImage::Format_RGB888).copy();
    }
    if(frame.channels()==4){
        cv::cvtColor(frame,converted,cv::COLOR_BGRA2RGBA);
        return QImage(converted.data,converted.cols,converted.rows,static_cast<int>(converted.step),QImage::Format_RGBA8888).copy();
    }
    return QImage(frame.data,frame.cols,frame.rows,static_cast<int>(frame.step),QImage::Format_Grayscale8).copy();
}
