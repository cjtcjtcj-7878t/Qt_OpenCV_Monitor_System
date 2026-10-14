#ifndef VIDEOPLAYERWORKER_H
#define VIDEOPLAYERWORKER_H

#include <QImage>
#include <QObject>
#include <QString>
#include <QTimer>

class videoplayer;

// 在独立线程中执行所有视频读取和 OpenCV 操作。
class videoplayerworker : public QObject
{
    Q_OBJECT

public:
    explicit videoplayerworker(QObject *parent = nullptr);
    ~videoplayerworker() override;

public slots:
    void initialize();
    void shutdown();
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
    void readNextFrame();

private:
    void updatePlaybackTimer();
    void emitCurrentFrame();

    videoplayer *video_player = nullptr;
    QTimer *playback_timer = nullptr;
    QString current_video_path;
    double playback_rate = 1.0;
    bool playing = false;
};

#endif // VIDEOPLAYERWORKER_H
