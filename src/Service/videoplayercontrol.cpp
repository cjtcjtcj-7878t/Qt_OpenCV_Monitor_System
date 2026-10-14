#include "videoplayercontrol.h"

#include "videoplayerworker.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>

videoplayercontrol::videoplayercontrol(QObject *parent)
    : QObject(parent),
      worker(new videoplayerworker)
{
    worker->moveToThread(&worker_thread);

    connect(&worker_thread,
            &QThread::started,
            worker,
            &videoplayerworker::initialize);
    connect(&worker_thread,
            &QThread::finished,
            worker,
            &QObject::deleteLater);

    connect(this,
            &videoplayercontrol::closeVideoRequested,
            worker,
            &videoplayerworker::closeVideo,
            Qt::QueuedConnection);
    connect(this,
            &videoplayercontrol::togglePlaybackRequested,
            worker,
            &videoplayerworker::togglePlayback,
            Qt::QueuedConnection);
    connect(this,
            &videoplayercontrol::playRequested,
            worker,
            &videoplayerworker::play,
            Qt::QueuedConnection);
    connect(this,
            &videoplayercontrol::pauseRequested,
            worker,
            &videoplayerworker::pause,
            Qt::QueuedConnection);
    connect(this,
            &videoplayercontrol::seekRequested,
            worker,
            &videoplayerworker::seek,
            Qt::QueuedConnection);
    connect(this,
            &videoplayercontrol::playbackRateRequested,
            worker,
            &videoplayerworker::setPlaybackRate,
            Qt::QueuedConnection);

    connect(worker,
            &videoplayerworker::frameReady,
            this,
            &videoplayercontrol::handleFrame,
            Qt::QueuedConnection);
    connect(worker,
            &videoplayerworker::positionChanged,
            this,
            &videoplayercontrol::handlePositionChanged,
            Qt::QueuedConnection);
    connect(worker,
            &videoplayerworker::durationChanged,
            this,
            &videoplayercontrol::handleDurationChanged,
            Qt::QueuedConnection);
    connect(worker,
            &videoplayerworker::playbackStateChanged,
            this,
            &videoplayercontrol::handlePlaybackStateChanged,
            Qt::QueuedConnection);
    connect(worker,
            &videoplayerworker::videoOpened,
            this,
            &videoplayercontrol::handleVideoOpened,
            Qt::QueuedConnection);
    connect(worker,
            &videoplayerworker::videoClosed,
            this,
            &videoplayercontrol::handleVideoClosed,
            Qt::QueuedConnection);
    connect(worker,
            &videoplayerworker::errorOccurred,
            this,
            &videoplayercontrol::handleError,
            Qt::QueuedConnection);

    worker_thread.start();
}

videoplayercontrol::~videoplayercontrol()
{
    if (worker_thread.isRunning()) {
        QMetaObject::invokeMethod(worker,"shutdown",Qt::BlockingQueuedConnection);
        worker_thread.quit();
        worker_thread.wait();
    }
}

void videoplayercontrol::openVideo(const QString &videoPath)
{
    // 先在界面线程检查并整理路径，后台线程收到的始终是实际文件的绝对路径。
    const QFileInfo fileInfo(videoPath.trimmed());
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit closeVideoRequested();
        emit errorOccurred(QString("视频文件不存在：\n%1").arg(videoPath.trimmed()));
        return;
    }

    const QString absolutePath = QDir::cleanPath(fileInfo.absoluteFilePath());
    if (!worker_thread.isRunning()) {
        emit errorOccurred("视频播放线程没有正常运行");
        return;
    }

    // 显式使用 QueuedConnection，保证 OpenCV 只在播放线程中执行。
    const bool invoked = QMetaObject::invokeMethod(
        worker,
        "openVideo",
        Qt::QueuedConnection,
        Q_ARG(QString, absolutePath));
    if (!invoked) {
        emit errorOccurred("无法向视频播放线程发送打开请求");
    }
}

void videoplayercontrol::closeVideo()
{
    emit closeVideoRequested();
}

void videoplayercontrol::togglePlayback()
{
    emit togglePlaybackRequested();
}

void videoplayercontrol::play()
{
    emit playRequested();
}

void videoplayercontrol::pause()
{
    emit pauseRequested();
}

void videoplayercontrol::seek(qint64 position)
{
    emit seekRequested(position);
}

void videoplayercontrol::setPlaybackRate(double rate)
{
    emit playbackRateRequested(rate);
}

bool videoplayercontrol::saveSnapshot(const QString &directory,QString &savedPath,QString &errorMessage,const QString &filePrefix)
{
    savedPath.clear();
    errorMessage.clear();

    if (!has_video || current_image.isNull()) {
        errorMessage = "当前没有可保存的视频画面!";
        return false;
    }

    const QString trimmedDirectory = directory.trimmed();
    if (trimmedDirectory.isEmpty()) {
        errorMessage = "图片保存路径为空，请先在设置中配置!";
        return false;
    }

    QDir saveDirectory(trimmedDirectory);
    if (!saveDirectory.exists() && !saveDirectory.mkpath(".")) {
        errorMessage = "图片保存目录创建失败!";
        return false;
    }

    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    const QString prefix=filePrefix.trimmed().isEmpty()?QString("snapshot"):filePrefix.trimmed();
    QString filePath;
    for (int index = 0; index < 1000; ++index) {
        const QString suffix = index == 0 ? QString() : QString("_%1").arg(index);
        filePath = saveDirectory.filePath(QString("%1_%2%3.png").arg(prefix,timestamp,suffix));
        if (!QFile::exists(filePath)) {
            break;
        }
    }

    if (!current_image.save(filePath, "PNG")) {
        errorMessage = "截屏图片保存失败!";
        return false;
    }

    savedPath = filePath;
    return true;
}

qint64 videoplayercontrol::duration() const
{
    return current_duration;
}

qint64 videoplayercontrol::position() const
{
    return current_position;
}

bool videoplayercontrol::isPlaying() const
{
    return current_playing;
}

void videoplayercontrol::handleFrame(const QImage &image)
{
    current_image = image;
    emit frameReady(image);
}

void videoplayercontrol::handlePositionChanged(qint64 position, qint64 duration)
{
    current_position = position;
    current_duration = duration;
    emit positionChanged(position, duration);
}

void videoplayercontrol::handleDurationChanged(qint64 duration)
{
    current_duration = duration;
    emit durationChanged(duration);
}

void videoplayercontrol::handlePlaybackStateChanged(bool playing)
{
    current_playing = playing;
    emit playbackStateChanged(playing);
}

void videoplayercontrol::handleVideoOpened(const QString &videoPath, qint64 duration)
{
    has_video = true;
    current_duration = duration;
    current_position = 0;
    emit videoOpened(videoPath, duration);
}

void videoplayercontrol::handleVideoClosed()
{
    has_video = false;
    current_image = QImage();
    current_duration = 0;
    current_position = 0;
    current_playing = false;
    emit videoClosed();
}

void videoplayercontrol::handleError(const QString &message)
{
    emit errorOccurred(message);
}
