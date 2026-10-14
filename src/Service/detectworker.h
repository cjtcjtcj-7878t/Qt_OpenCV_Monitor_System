#ifndef DETECTWORKER_H
#define DETECTWORKER_H

#include <QDateTime>
#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include "../Model/recordingtimeline.h"
#include <QString>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "../Yolo/Detect.h"

// 移动侦测线程：负责检测、画框和写 detect_t 对应的视频文件。
class detectworker : public QObject
{
    Q_OBJECT

public:
    explicit detectworker(QObject *parent = nullptr);
    ~detectworker() override;

    // 这个函数只修改原子变量，主线程可以安全地立即通知检测线程停止接收旧帧。
    void requestFrameAcceptance(bool accepted) noexcept;
    // 每个通道只允许一个待处理帧，避免检测速度较慢时队列无限增长。
    bool tryReserveFrame(int viewIndex) noexcept;

public slots:
    void initialize();
    void shutdown();
    void setActive(bool active);
    void flushSegments();
    void processFrame(int viewIndex,
                      const cv::Mat &frame,
                      int cameraId,
                      double fps,
                      int intervalSeconds,
                      const QString &videoDirectory,
                      const QString &imageDirectory);

signals:
    void frameReady(int viewIndex,const QImage &image);
    void recordingStarted(int cameraId,
                          const QString &videoPath,
                          const QDateTime &startTime);
    void recordingFinished(int cameraId,
                           const QString &videoPath,
                           qint64 fileSize,
                           const QDateTime &endTime);
    void automaticSnapshotFinished(int cameraId,
                                   const QString &imagePath,
                                   const QString &videoPath);
    void segmentsFlushed();
    void errorOccurred(int viewIndex,const QString &message);

private:
    static constexpr int channel_count = 4;

    struct detect_session
    {
        cv::VideoWriter writer;
        int camera_id = 0;
        double fps = 25.0;
        int recording_interval_seconds = 30;
        QString video_directory;
        QString image_directory;
        QString current_video_path;
        QDateTime segment_start;
        QElapsedTimer segment_timer;
        RecordingTimeline<cv::Mat> timeline;
        int frame_count = 0;
        int startup_snapshot_attempts = 0; // 每次开启侦测仅截取前三个可录制帧
        bool detection_event_active = false;
    };

    bool isValidIndex(int viewIndex) const;
    void loadDetector();
    bool startSegment(int viewIndex,const cv::Size &frameSize);
    void finishSegment(int viewIndex);
    void saveAutomaticSnapshot(int viewIndex,const cv::Mat &frame);
    QImage matToImage(const cv::Mat &frame) const;

    std::array<detect_session,channel_count> sessions;
    std::array<std::vector<YOLO_OUT>,channel_count> detection_results;
    std::unique_ptr<Detect> detector;
    bool initialized = false;
    bool detector_load_attempted = false;
    bool active = false;
    // 主线程切换模式时先修改这个标志，让检测线程快速丢弃排队旧帧。
    std::atomic_bool accepting_frames{false};
    std::array<std::atomic_bool,channel_count> frame_pending{};
};

#endif // DETECTWORKER_H
