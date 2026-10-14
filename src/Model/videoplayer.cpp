#include "videoplayer.h"

#include <QFileInfo>

#include <cmath>

videoplayer::videoplayer() = default;

videoplayer::~videoplayer()
{
    close();
}

bool videoplayer::open(const QString &videoPath, QString *errorMessage)
{
    if (errorMessage) {
        errorMessage->clear();
    }

    close();

    const QString trimmedPath = videoPath.trimmed();
    const QFileInfo fileInfo(trimmedPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QString("视频文件不存在:\n%1").arg(trimmedPath);
        }
        return false;
    }

    const QString absolutePath = fileInfo.absoluteFilePath();
    const QByteArray encodedPath = absolutePath.toUtf8();

    // 录像由本项目使用 MJPG/AVI 保存，优先指定 FFmpeg；某些机器没有
    // FFmpeg 插件时再退回 OpenCV 的自动后端选择。
    bool opened = video_capture.open(encodedPath.constData(), cv::CAP_FFMPEG);
    if (!opened) {
        opened = video_capture.open(encodedPath.constData(), cv::CAP_ANY);
    }
    if (!opened) {
        if (errorMessage) {
            *errorMessage = QString("OpenCV无法打开视频文件：\n%1").arg(absolutePath);
        }
        close();
        return false;
    }

    video_fps = video_capture.get(cv::CAP_PROP_FPS);
    if (video_fps <= 0.0 || !std::isfinite(video_fps)) {
        video_fps = 25.0;
    }

    const double reportedFrameCount = video_capture.get(cv::CAP_PROP_FRAME_COUNT);
    if (reportedFrameCount > 0.0 && std::isfinite(reportedFrameCount)) {
        video_frame_count = static_cast<qint64>(std::llround(reportedFrameCount));
    } else {
        video_frame_count = 0;
    }

    current_frame_index = -1;
    return true;
}

void videoplayer::close()
{
    if (video_capture.isOpened()) {
        video_capture.release();
    }

    current_frame.release();
    video_fps = 0.0;
    video_frame_count = 0;
    current_frame_index = -1;
}



bool videoplayer::isOpened() const
{
    return video_capture.isOpened();
}

bool videoplayer::readNextFrame()
{
    if (!video_capture.isOpened()) {
        return false;
    }

    cv::Mat frame;
    if (!video_capture.read(frame)) {
        return false;
    }

    current_frame = frame;
    ++current_frame_index;
    return true;
}

bool videoplayer::seekToPosition(qint64 position)
{
    if (!video_capture.isOpened() || video_fps <= 0.0) {
        return false;
    }

    const qint64 requestedFrame = static_cast<qint64>(
        (static_cast<double>(qMax<qint64>(0, position)) / 1000.0) * video_fps);
    const qint64 targetFrame = video_frame_count > 0? qBound<qint64>(qint64(0),requestedFrame,video_frame_count - 1): qMax<qint64>(qint64(0), requestedFrame);

    if (!video_capture.set(cv::CAP_PROP_POS_FRAMES, static_cast<double>(targetFrame))) {
        return false;
    }

    cv::Mat frame;
    if (!video_capture.read(frame)) {
        return false;
    }

    current_frame = frame;
    current_frame_index = targetFrame;
    return true;
}

const cv::Mat &videoplayer::currentFrame() const
{
    return current_frame;
}

double videoplayer::fps() const
{
    return video_fps;
}

qint64 videoplayer::frameCount() const
{
    return video_frame_count;
}

qint64 videoplayer::currentFrameIndex() const
{
    return current_frame_index;
}

qint64 videoplayer::duration() const
{
    if (video_fps <= 0.0 || video_frame_count <= 0) {
        return 0;
    }

    return static_cast<qint64>((static_cast<double>(video_frame_count) / video_fps) * 1000.0);
}

qint64 videoplayer::position() const
{
    if (video_fps <= 0.0 || current_frame_index < 0) {
        return 0;
    }

    return static_cast<qint64>((static_cast<double>(current_frame_index) / video_fps) * 1000.0);
}
