#ifndef VIDEOPLAYERCONTROL_H
#define VIDEOPLAYERCONTROL_H

#include <QImage>
#include <QObject>
#include <QThread>
#include <QString>

class videoplayerworker;

// 界面线程中的播放控制门面，不直接接触 OpenCV。
class videoplayercontrol : public QObject
{
    Q_OBJECT

public:
    explicit videoplayercontrol(QObject *parent = nullptr);
    ~videoplayercontrol() override;

    bool saveSnapshot(const QString &directory,QString &savedPath,QString &errorMessage,const QString &filePrefix="snapshot");
    qint64 duration() const;
    qint64 position() const;
    bool isPlaying() const;

public slots:
    void openVideo(const QString &videoPath);
    void closeVideo();
    void togglePlayback();
    void play();
    void pause();
    void seek(qint64 position);
    void setPlaybackRate(double rate);

signals:
    void frameReady(const QImage &image);
    void positionChanged(qint64 position, qint64 duration);
    void durationChanged(qint64 duration);
    void playbackStateChanged(bool playing);
    void videoOpened(const QString &videoPath, qint64 duration);
    void videoClosed();
    void errorOccurred(const QString &message);

private slots:
    void handleFrame(const QImage &image);
    void handlePositionChanged(qint64 position, qint64 duration);
    void handleDurationChanged(qint64 duration);
    void handlePlaybackStateChanged(bool playing);
    void handleVideoOpened(const QString &videoPath, qint64 duration);
    void handleVideoClosed();
    void handleError(const QString &message);

signals:
    void closeVideoRequested();
    void togglePlaybackRequested();
    void playRequested();
    void pauseRequested();
    void seekRequested(qint64 position);
    void playbackRateRequested(double rate);

private:
    QThread worker_thread;
    videoplayerworker *worker = nullptr;
    QImage current_image;
    qint64 current_duration = 0;
    qint64 current_position = 0;
    bool current_playing = false;
    bool has_video = false;
};

#endif // VIDEOPLAYERCONTROL_H
