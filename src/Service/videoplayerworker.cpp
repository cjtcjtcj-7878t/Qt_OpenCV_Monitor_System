#include "videoplayerworker.h"

#include "../Model/videoplayer.h"

#include <QThread>
#include <QTimer>
#include <QFileInfo>

#include <cmath>
#include <opencv2/imgproc.hpp>

namespace
{
QImage matToImage(const cv::Mat &frame);
}

videoplayerworker::videoplayerworker(QObject *parent)
    : QObject(parent),
    video_player(new videoplayer)
{
}

videoplayerworker::~videoplayerworker()
{
    if (playback_timer) {
        playback_timer->stop();
    }
    video_player->close();
    delete video_player;
}

void videoplayerworker::initialize()
{
    if (playback_timer) {
        return;
    }

    playback_timer = new QTimer(this);
    playback_timer->setTimerType(Qt::PreciseTimer);
    connect(playback_timer,&QTimer::timeout,this,&videoplayerworker::readNextFrame);
}

void videoplayerworker::shutdown()
{
    closeVideo();
    QThread::currentThread()->quit();
}

void videoplayerworker::openVideo(const QString &videoPath)
{
    if (playback_timer) {
        playback_timer->stop();
    }
    if (playing) {
        playing = false;
        emit playbackStateChanged(false);
    }

    video_player->close();
    current_video_path.clear();

    const QString absolutePath = QFileInfo(videoPath.trimmed()).absoluteFilePath();
    QString errorMessage;
    if (!video_player->open(absolutePath, &errorMessage)) {
        emit videoClosed();
        emit errorOccurred(errorMessage);
        return;
    }

    if (!video_player->readNextFrame()) {
        video_player->close();
        emit videoClosed();
        emit errorOccurred(QString("视频中没有可读取的画面：\n%1").arg(absolutePath));
        return;
    }

    current_video_path = absolutePath;
    const qint64 videoDuration = video_player->duration();
    emit videoOpened(current_video_path, videoDuration);
    emit durationChanged(videoDuration);
    emitCurrentFrame();
    emit positionChanged(video_player->position(), videoDuration);

    playing = true;
    emit playbackStateChanged(true);
    updatePlaybackTimer();
}

void videoplayerworker::closeVideo()
{
    const bool hadVideo = video_player->isOpened() || !current_video_path.isEmpty();
    if (playback_timer) {
        playback_timer->stop();
    }

    if (playing) {
        playing = false;
        emit playbackStateChanged(false);
    }

    video_player->close();
    current_video_path.clear();

    if (hadVideo) {
        emit videoClosed();
    }
}

void videoplayerworker::togglePlayback()
{
    if (!video_player->isOpened()) {
        return;
    }

    if (playing) {
        pause();
    } else {
        play();
    }
}

void videoplayerworker::play()
{
    if (!video_player->isOpened()) {
        return;
    }

    if (video_player->frameCount() > 0
        && video_player->currentFrameIndex() >= video_player->frameCount() - 1) {
        if (!video_player->seekToPosition(0)) {
            emit errorOccurred("视频无法回到开始位置!");
            return;
        }
        emitCurrentFrame();
        emit positionChanged(video_player->position(), video_player->duration());
    }

    if (!playing) {
        playing = true;
        emit playbackStateChanged(true);
    }
    updatePlaybackTimer();
}

void videoplayerworker::pause()
{
    if (playback_timer) {
        playback_timer->stop();
    }
    if (!playing) {
        return;
    }

    playing = false;
    emit playbackStateChanged(false);
}

void videoplayerworker::seek(qint64 position)
{
    if (!video_player->isOpened()) {
        return;
    }

    if (!video_player->seekToPosition(position)) {
        emit errorOccurred("视频进度跳转失败!");
        return;
    }

    emitCurrentFrame();
    emit positionChanged(video_player->position(), video_player->duration());
}

void videoplayerworker::setPlaybackRate(double rate)
{
    if (!std::isfinite(rate) || rate <= 0.0) {
        return;
    }

    playback_rate = rate;
    updatePlaybackTimer();
}

void videoplayerworker::readNextFrame()
{
    if (!video_player->isOpened() || !playing) {
        return;
    }

    if (!video_player->readNextFrame()) {
        if (playback_timer) {
            playback_timer->stop();
        }
        playing = false;
        emit playbackStateChanged(false);
        emit positionChanged(video_player->duration(), video_player->duration());
        return;
    }

    emitCurrentFrame();
    emit positionChanged(video_player->position(), video_player->duration());
}

void videoplayerworker::updatePlaybackTimer()
{
    if (!playback_timer || !video_player->isOpened() || !playing) {
        return;
    }

    double fps = video_player->fps();
    if (!std::isfinite(fps) || fps <= 0.0) fps = 25.0;
    const double interval = 1000.0 / (fps * playback_rate);
    playback_timer->start(qMax(1, static_cast<int>(std::round(interval))));
}

void videoplayerworker::emitCurrentFrame()
{
    const QImage image = matToImage(video_player->currentFrame());
    if (!image.isNull()) {
        emit frameReady(image);
    }
}

namespace
{
QImage matToImage(const cv::Mat &frame)
{
    //opencv中的Mat类转换为Qt中的QImage类,将opencv捕捉的画面显示在Qt界面上
    if (frame.empty()) {
        return QImage();
    }

    cv::Mat converted;
    if (frame.channels() == 1) {
        return QImage(frame.data,frame.cols,frame.rows,static_cast<int>(frame.step),QImage::Format_Grayscale8).copy();
    }

    if (frame.channels() == 3) {
        cv::cvtColor(frame, converted, cv::COLOR_BGR2RGB);
        return QImage(converted.data,converted.cols,converted.rows,static_cast<int>(converted.step),QImage::Format_RGB888).copy();
    }

    if (frame.channels() == 4) {
        cv::cvtColor(frame, converted, cv::COLOR_BGRA2RGBA);
        return QImage(converted.data,converted.cols,converted.rows,static_cast<int>(converted.step),QImage::Format_RGBA8888).copy();
    }

    return QImage();
}
}
