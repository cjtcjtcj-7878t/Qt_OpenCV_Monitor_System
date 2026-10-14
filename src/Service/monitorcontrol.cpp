#include "monitorcontrol.h"

#include "monitorworker.h"
#include "detectworker.h"
#include "videocontrol.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QMetaObject>

#include <QMetaType>

monitorcontrol::monitorcontrol(QObject *parent)
    : QObject(parent),
    worker(new monitorworker),
    detection_worker(new detectworker)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
    worker->moveToThread(&worker_thread);
    detection_worker->moveToThread(&detection_thread);

    //开启监控线程
    connect(&worker_thread,
            &QThread::started,
            worker,
            &monitorworker::initialize);
    connect(&worker_thread,
            &QThread::finished,
            worker,
            &QObject::deleteLater);
    //开启移动侦测线程
    connect(&detection_thread,
            &QThread::started,
            detection_worker,
            &detectworker::initialize);
    connect(&detection_thread,
            &QThread::finished,
            detection_worker,
            &QObject::deleteLater);

    //开启特定的摄像头
    connect(this,
            &monitorcontrol::openCameraRequested,
            worker,
            &monitorworker::openCamera,
            Qt::QueuedConnection);
    //关闭特定的摄像头
    connect(this,
            &monitorcontrol::closeCameraRequested,
            worker,
            &monitorworker::closeCamera,
            Qt::QueuedConnection);
    //关闭所有摄像头
    connect(this,
            &monitorcontrol::closeAllCamerasRequested,
            worker,
            &monitorworker::closeAllCameras,
            Qt::QueuedConnection);
    connect(this,
            &monitorcontrol::flushSegmentsRequested,
            worker,
            &monitorworker::flushSegments,
            Qt::QueuedConnection);
    connect(this,
            &monitorcontrol::flushDetectionSegmentsRequested,
            detection_worker,
            &detectworker::flushSegments,
            Qt::QueuedConnection);
    //手动截图
    connect(this,
            &monitorcontrol::takeSnapshotRequested,
            worker,
            &monitorworker::takeSnapshot,
            Qt::QueuedConnection);

    connect(this,
            &monitorcontrol::monitorDetectionModeRequested,
            worker,
            &monitorworker::setDetectionMode,
            Qt::QueuedConnection);

    connect(this,
            &monitorcontrol::detectionModeRequested,
            detection_worker,
            &detectworker::setActive,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::frameForDetection,
            this,
            &monitorcontrol::handleFrameForDetection,
            Qt::QueuedConnection);

    connect(this,
            &monitorcontrol::detectionFrameRequested,
            detection_worker,
            &detectworker::processFrame,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::frameReady,
            this,
            &monitorcontrol::handleFrame,
            Qt::QueuedConnection);

    connect(detection_worker,
            &detectworker::frameReady,
            this,
            &monitorcontrol::handleDetectionFrame,
            Qt::QueuedConnection);

    connect(detection_worker,
            &detectworker::recordingStarted,
            this,
            &monitorcontrol::handleDetectionRecordingStarted,
            Qt::QueuedConnection);

    connect(detection_worker,
            &detectworker::recordingFinished,
            this,
            &monitorcontrol::handleDetectionRecordingFinished,
            Qt::QueuedConnection);

    connect(detection_worker,
            &detectworker::automaticSnapshotFinished,
            this,
            &monitorcontrol::handleAutomaticSnapshotFinished,
            Qt::QueuedConnection);

    connect(detection_worker,
            &detectworker::errorOccurred,
            this,
            &monitorcontrol::handleError,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::cameraStateChanged,
            this,
            &monitorcontrol::handleCameraStateChanged,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::recordingStarted,
            this,
            &monitorcontrol::handleRecordingStarted,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::recordingFinished,
            this,
            &monitorcontrol::handleRecordingFinished,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::snapshotFinished,
            this,
            &monitorcontrol::handleSnapshotFinished,
            Qt::QueuedConnection);

    connect(worker,
            &monitorworker::errorOccurred,
            this,
            &monitorcontrol::handleError,
            Qt::QueuedConnection);

    //线程开启
    worker_thread.start();
    detection_thread.start();
}

monitorcontrol::~monitorcontrol()
{
    if(worker_thread.isRunning()){
        closeAllCamerasAndWait();
        QMetaObject::invokeMethod(worker,"shutdown",Qt::BlockingQueuedConnection);
        worker_thread.quit();
        worker_thread.wait();
    }
    if(detection_thread.isRunning()){
        QMetaObject::invokeMethod(detection_worker,"setActive",Qt::BlockingQueuedConnection,Q_ARG(bool,false));
        QMetaObject::invokeMethod(detection_worker,"shutdown",Qt::BlockingQueuedConnection);
        detection_thread.quit();
        detection_thread.wait();
    }
}

void monitorcontrol::openCamera(int viewIndex,int deviceIndex,int channelId,int intervalSeconds,const QString &videoDirectory,const QString &imageDirectory)
{
    if(deviceIndex < 0){
        // -1、-2……仅在请求中编码虚拟设备顺序，配置中的编号保持不变。
        QMetaObject::invokeMethod(worker,"openVirtualCamera",Qt::QueuedConnection,Q_ARG(int,viewIndex),Q_ARG(int,-deviceIndex-1));
        return;
    }
    int cameraId=0;
    int cameraIndex=-1;
    QString errorMessage;
    if(!camera_model.findByChannel(channelId,cameraId,cameraIndex,&errorMessage)){
        emit errorOccurred(viewIndex,QString("无法找到通道%1对应的数据库摄像头记录：\n%2").arg(channelId).arg(errorMessage));
        return;
    }

    current_image_directory=imageDirectory;
    Q_UNUSED(deviceIndex)
    emit openCameraRequested(viewIndex,cameraIndex,cameraId,intervalSeconds,videoDirectory,imageDirectory);
}

void monitorcontrol::closeCamera(int viewIndex)
{
    emit closeCameraRequested(viewIndex);
}

void monitorcontrol::closeAllCameras()
{
    if(detection_mode){
        // 关闭摄像头时同时关闭移动侦测分段，避免线程退出后留下未完成文件。
        setDetectionMode(false);
    }
    emit closeAllCamerasRequested(++next_close_request_id);
}

void monitorcontrol::closeAllCamerasAndWait()
{
    if(!worker_thread.isRunning()){
        return;
    }

    const quint64 requestId=++next_close_request_id;
    QEventLoop waitLoop;
    const QMetaObject::Connection connection=connect(
        worker,
        &monitorworker::closeCompleted,
        &waitLoop,
        [requestId,&waitLoop](quint64 completedId){
            if(completedId==requestId){
                waitLoop.quit();
            }
        },
        Qt::QueuedConnection);
    emit closeAllCamerasRequested(requestId);
    waitLoop.exec();
    disconnect(connection);
}

void monitorcontrol::flushCurrentRecordings()
{
    // 当前录像仍在后台继续，先只封口这一段，下一帧会自动创建新的分段。
    if(detection_mode){
        emit flushDetectionSegmentsRequested();
    }else{
        emit flushSegmentsRequested();
    }
}

void monitorcontrol::takeSnapshot(int viewIndex)
{
    const QString directory =settinginfo::Image_file_path.trimmed();


    if (directory.isEmpty()) {
        emit errorOccurred(viewIndex, "图片保存路径为空，请先保存系统设置！");
        return;
    }
    current_image_directory = directory;
    emit takeSnapshotRequested(viewIndex, directory);
}

void monitorcontrol::takeAllSnapshots()
{
    const QString directory=settinginfo::Image_file_path.trimmed();

    if (directory.isEmpty()) {
        emit errorOccurred(-1, "图片保存路径为空，请先保存系统设置！");
        return;
    }

    current_image_directory = directory;
    emit takeSnapshotRequested(-1, directory);
}

void monitorcontrol::setDetectionMode(bool enabled)
{
    if(detection_mode==enabled){
        return;
    }

    detection_mode=enabled;
    // 两个工作线程接收同一个模式开关：一个停止普通录像，一个开始移动侦测录像。
    detection_worker->requestFrameAcceptance(enabled);
    emit monitorDetectionModeRequested(enabled);
    emit detectionModeRequested(enabled);
    emit detectionModeChanged(enabled);
}

void monitorcontrol::handleFrame(int viewIndex,const QImage &image)
{
    emit frameReady(viewIndex,image);
}

void monitorcontrol::handleFrameForDetection(int viewIndex,const cv::Mat &frame,int cameraId,double fps,int intervalSeconds,const QString &videoDirectory,
                                             const QString &imageDirectory)
{
    Q_UNUSED(imageDirectory)
    if (!detection_mode || !detection_worker->tryReserveFrame(viewIndex)) {
        return;
    }
    emit detectionFrameRequested(viewIndex,frame,cameraId,fps,intervalSeconds,videoDirectory,settinginfo::Image_file_path.trimmed());
}

void monitorcontrol::handleDetectionFrame(int viewIndex,const QImage &image)
{
    emit detectionFrameReady(viewIndex,image);
}

void monitorcontrol::handleCameraStateChanged(int viewIndex,bool opened)
{
    emit cameraStateChanged(viewIndex,opened);
}

void monitorcontrol::handleRecordingStarted(int cameraId,const QString &videoPath,const QDateTime &startTime)
{
    const QFileInfo fileInfo(videoPath);
    videocontrol databaseControl;
    int videoId=0;
    QString errorMessage;
    if(!databaseControl.createVideoRecord(cameraId,fileInfo.fileName(),fileInfo.absoluteFilePath(),startTime,videoId,errorMessage)){
        emit errorOccurred(-1,QString("录像文件已创建，但写入video_t失败：\n%1").arg(errorMessage));
        return;
    }
    video_ids.insert(videoPath,videoId);
}

void monitorcontrol::handleRecordingFinished(int cameraId,const QString &videoPath,qint64 fileSize,const QDateTime &endTime)
{
    Q_UNUSED(cameraId)
    const int videoId=video_ids.take(videoPath);
    if(videoId<=0){
        emit errorOccurred(-1,QString("录像已保存，但找不到video_t记录：\n%1").arg(videoPath));
        return;
    }

    videocontrol databaseControl;
    QString errorMessage;
    if(!databaseControl.finishVideoRecord(videoId,fileSize,endTime,errorMessage)){
        emit errorOccurred(-1,QString("录像已保存，但更新video_t失败：\n%1").arg(errorMessage));
        return;
    }
    emit videoSaved(videoPath);
}

void monitorcontrol::handleDetectionRecordingStarted(int cameraId,const QString &videoPath,const QDateTime &startTime)
{
    const QFileInfo fileInfo(videoPath);
    videocontrol databaseControl;
    int videoId=0;
    QString errorMessage;
    if(!databaseControl.createDetectVideoRecord(cameraId,fileInfo.fileName(),fileInfo.absoluteFilePath(),startTime,videoId,errorMessage)){
        emit errorOccurred(-1,QString("移动侦测录像已创建，但写入detect_t失败：\n%1").arg(errorMessage));
        return;
    }
    detect_video_ids.insert(videoPath,videoId);
    current_detect_video_paths.insert(cameraId,videoPath);
}

void monitorcontrol::handleDetectionRecordingFinished(int cameraId,const QString &videoPath,qint64 fileSize,const QDateTime &endTime)
{
    const int videoId=detect_video_ids.take(videoPath);
    if(videoId<=0){
        emit errorOccurred(-1,QString("移动侦测录像已保存，但找不到detect_t记录：\n%1").arg(videoPath));
        return;
    }
    videocontrol databaseControl;
    QString errorMessage;
    if(!databaseControl.finishDetectVideoRecord(videoId,fileSize,endTime,errorMessage)){
        emit errorOccurred(-1,QString("移动侦测录像已保存，但更新detect_t失败：\n%1").arg(errorMessage));
        return;
    }
    if(current_detect_video_paths.value(cameraId)==videoPath){
        current_detect_video_paths.remove(cameraId);
    }
    emit detectVideoSaved(videoPath);
}

void monitorcontrol::handleSnapshotFinished(int cameraId,const QString &imagePath,const QString &videoPath)
{
    QString relatedVideoPath=videoPath;
    int videoId=video_ids.value(relatedVideoPath,0);
    if(videoId<=0&&detection_mode){
        // 移动侦测录像由另一个线程管理，截图事件需要关联当前 detect_t 视频。
        relatedVideoPath=current_detect_video_paths.value(cameraId);
        videoId=detect_video_ids.value(relatedVideoPath,0);
    }
    if(videoId<=0){
        emit errorOccurred(-1,"截图文件已保存，但当前录像还没有对应的数据库编号!");
        return;
    }

    videocontrol databaseControl;
    QString errorMessage;
    if(!databaseControl.createImageRecord(cameraId,videoId,imagePath,0,errorMessage)){
        emit errorOccurred(-1,QString("截图文件已保存，但写入image_t失败：\n%1").arg(errorMessage));
        return;
    }
    emit snapshotSaved(imagePath);
}

void monitorcontrol::handleAutomaticSnapshotFinished(
    int cameraId,
    const QString &imagePath,
    const QString &videoPath)
{
    // 自动截图来自检测线程，必须关联发出截图时正在录制的 detect_t 视频。
    const int videoId=detect_video_ids.value(videoPath,0);
    if(videoId<=0){
        emit errorOccurred(-1,QString("自动截图已保存，但找不到对应的detect_t记录：\n%1").arg(videoPath));
        return;
    }

    videocontrol databaseControl;
    QString errorMessage;
    if(!databaseControl.createImageRecord(cameraId,videoId,imagePath,1,errorMessage)){
        emit errorOccurred(-1,QString("自动截图已保存，但写入image_t失败：\n%1").arg(errorMessage));
        return;
    }
    emit snapshotSaved(imagePath);
}

void monitorcontrol::handleError(int viewIndex,const QString &message)
{
    emit errorOccurred(viewIndex,message);
}
