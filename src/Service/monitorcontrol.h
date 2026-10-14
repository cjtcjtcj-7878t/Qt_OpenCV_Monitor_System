#ifndef MONITORCONTROL_H
#define MONITORCONTROL_H

#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QThread>
#include <QString>

#include <opencv2/core.hpp>
#include"../Model/settinginfo.h"
#include "../Model/cameramodel.h"

class monitorworker;    //监控线程
class detectworker;     //移动侦测线程

class monitorcontrol : public QObject
{
    Q_OBJECT

public:
    explicit monitorcontrol(QObject *parent = nullptr);
    ~monitorcontrol() override;
    void closeAllCamerasAndWait();

public slots:
    void openCamera(int viewIndex,
                    int deviceIndex,
                    int channelId,
                    int intervalSeconds,
                    const QString &videoDirectory,
                    const QString &imageDirectory);
    void closeCamera(int viewIndex);
    void closeAllCameras();
    void flushCurrentRecordings();
    void takeSnapshot(int viewIndex);
    void takeAllSnapshots();
    void setDetectionMode(bool enabled);

signals:
    void frameReady(int viewIndex,const QImage &image);
    void detectionFrameReady(int viewIndex,const QImage &image);
    void cameraStateChanged(int viewIndex,bool opened);
    void videoSaved(const QString &videoPath);
    void detectVideoSaved(const QString &videoPath);
    void snapshotSaved(const QString &imagePath);
    void detectionModeChanged(bool enabled);
    void errorOccurred(int viewIndex,const QString &message);

private slots:
    void handleFrame(int viewIndex,const QImage &image);
    void handleFrameForDetection(int viewIndex,
                                 const cv::Mat &frame,
                                 int cameraId,
                                 double fps,
                                 int intervalSeconds,
                                 const QString &videoDirectory,
                                 const QString &imageDirectory);
    void handleDetectionFrame(int viewIndex,const QImage &image);
    void handleCameraStateChanged(int viewIndex,bool opened);
    void handleRecordingStarted(int cameraId,
                                const QString &videoPath,
                                const QDateTime &startTime);
    void handleRecordingFinished(int cameraId,
                                 const QString &videoPath,
                                 qint64 fileSize,
                                 const QDateTime &endTime);
    void handleDetectionRecordingStarted(int cameraId,
                                         const QString &videoPath,
                                         const QDateTime &startTime);
    void handleDetectionRecordingFinished(int cameraId,
                                          const QString &videoPath,
                                          qint64 fileSize,
                                          const QDateTime &endTime);
    void handleSnapshotFinished(int cameraId,
                                const QString &imagePath,
                                const QString &videoPath);
    void handleAutomaticSnapshotFinished(int cameraId,
                                         const QString &imagePath,
                                         const QString &videoPath);
    void handleError(int viewIndex,const QString &message);

signals:
    void openCameraRequested(int viewIndex,
                             int deviceIndex,
                             int cameraId,
                             int intervalSeconds,
                             const QString &videoDirectory,
                             const QString &imageDirectory);
    void closeCameraRequested(int viewIndex);
    void closeAllCamerasRequested(quint64 requestId);
    void flushSegmentsRequested();
    void flushDetectionSegmentsRequested();
    void detectionFrameRequested(int viewIndex,
                                 const cv::Mat &frame,
                                 int cameraId,
                                 double fps,
                                 int intervalSeconds,
                                 const QString &videoDirectory,
                                 const QString &imageDirectory);
    void takeSnapshotRequested(int viewIndex,const QString &imageDirectory);
    void detectionModeRequested(bool enabled);
    void monitorDetectionModeRequested(bool enabled);

private:
    QThread worker_thread;
    monitorworker *worker = nullptr;
    QThread detection_thread;
    detectworker *detection_worker = nullptr;
    QString current_image_directory;
    QHash<QString,int> video_ids;
    QHash<QString,int> detect_video_ids;
    QHash<int,QString> current_detect_video_paths;
    quint64 next_close_request_id = 0;
    cameramodel camera_model;
    bool detection_mode = false;    //默认不开启移动侦测模式
};

#endif // MONITORCONTROL_H
