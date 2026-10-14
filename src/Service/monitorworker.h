#ifndef MONITORWORKER_H
#define MONITORWORKER_H

#include <array>

#include <QDateTime>
#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include "../Model/recordingtimeline.h"
#include <QString>
#include <QSaveFile>
#include <QDebug>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include "../Model/cameracapture.h"

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

Q_DECLARE_METATYPE(cv::Mat)

class QTimer;

class monitorworker : public QObject
{
    Q_OBJECT

public:
    explicit monitorworker(QObject *parent = nullptr);
    ~monitorworker() override;

public slots:
    void initialize();
    void shutdown();
    void openCamera(int viewIndex,
                    int deviceIndex,
                    int cameraId,
                    int intervalSeconds,
                    const QString &videoDirectory,
                    const QString &imageDirectory);
    void openVirtualCamera(int viewIndex, int virtualIndex);
    void closeCamera(int viewIndex);
    void closeAllCameras(quint64 requestId = 0);
    void flushSegments();
    void takeSnapshot(int viewIndex,const QString &imageDirectory);
    void setDetectionMode(bool enabled);

signals:
    void frameReady(int viewIndex,const QImage &image);
    void frameForDetection(int viewIndex,
                           const cv::Mat &frame,
                           int cameraId,
                           double fps,
                           int intervalSeconds,
                           const QString &videoDirectory,
                           const QString &imageDirectory);
    void cameraStateChanged(int viewIndex,bool opened);
    void closeCompleted(quint64 requestId);
    void segmentsFlushed();
    void recordingStarted(int cameraId,
                          const QString &videoPath,
                          const QDateTime &startTime);
    void recordingFinished(int cameraId,
                           const QString &videoPath,
                           qint64 fileSize,
                           const QDateTime &endTime);
    void snapshotFinished(int cameraId,
                          const QString &imagePath,
                          const QString &videoPath);
    void errorOccurred(int viewIndex,const QString &message);

private slots:
    void readNextFrames();

private:
    static constexpr int channel_count = 4;

    struct channel_session
    {
        cameracapture camera_capture;
        cv::VideoCapture virtual_capture;
        QString virtual_video_path;
        cv::Mat current_frame;
        int camera_id = 0;
        int recording_interval_seconds = 30;
        int frame_interval_ms = 40;
        QString video_directory;
        QString image_directory;
        QString current_video_path;
        QDateTime current_segment_start;
        QElapsedTimer segment_timer;
        RecordingTimeline<cv::Mat> timeline;
        QElapsedTimer frame_timer;
    };

    bool isValidIndex(int viewIndex) const;
    bool hasOpenedCamera() const;
    void updateCaptureTimer();
    void closeChannel(int viewIndex);
    bool startSegment(int viewIndex,const cv::Size &frameSize);
    void finishSegment(int viewIndex);
    void emitCurrentFrame(int viewIndex,const cv::Mat &frame);

    std::array<channel_session,channel_count> channels;
    QTimer *capture_timer = nullptr;
    bool detection_mode = false;
};

#endif // MONITORWORKER_H
