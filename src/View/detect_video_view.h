#ifndef DETECT_VIDEO_VIEW_H
#define DETECT_VIDEO_VIEW_H

#include <QCalendarWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QEvent>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QLocale>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>



class QFileSystemWatcher;
#include"../Model/settinginfo.h"
#include"../Service/videocontrol.h"
#include"../Service/videoplayercontrol.h"
#include"../Service/videoplayerworker.h"
#include "../Service/videoplayercontrol.h"

class detect_video_view : public QWidget
{
    Q_OBJECT

public:
    explicit detect_video_view(QWidget *parent = nullptr);

public slots:
    void refreshVideoList();
    void stopPlayback();

private:
    void init_qss();    //qss优化界面
    void init_left_widget();
    void init_right_widget();
    void init_connect();
    void updateVideoList();
    void playVideo(const QString &videoPath);
    void updatePlayPauseButton(bool playing);
    void updatePlaybackPosition(qint64 position,qint64 duration);
    void showCurrentFrame(const QImage &image);
    void seekSliderByMouse(QMouseEvent *event);
    QString formatDuration(qint64 milliseconds) const;
    void showWarningMessageBox(const QString &title,const QString &text);
    void updateVideoDirectoryWatcher();

    QWidget *left_widget = nullptr;
    QComboBox *channel_choose = nullptr;
    QListWidget *video_list = nullptr;
    QDateEdit *time_choose = nullptr;
    QCalendarWidget *calendar = nullptr;
    QLabel *page_label = nullptr;
    QPushButton *btn_last = nullptr;
    QPushButton *btn_next = nullptr;
    int current_page = 1;
    int total_page = 1;
    int page_size = 17;
    bool changing_page = false;

    QWidget *right_widget = nullptr;
    QLabel *video_label = nullptr;
    QLabel *empty_label = nullptr;
    QStackedLayout *player_stack = nullptr;
    QSlider *progress_slider = nullptr;
    QLabel *time_label = nullptr;
    QPushButton *play_pause_button = nullptr;
    QPushButton *snapshot_button = nullptr;
    QComboBox *speed_choose = nullptr;
    videoplayercontrol *player_control = nullptr;
    QImage current_image;
    int current_video_id = 0;
    int current_camera_id = 0;
    QFileSystemWatcher *video_directory_watcher = nullptr;
    QString watched_video_directory;

protected:
    bool eventFilter(QObject *watched,QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
};

#endif // DETECT_VIDEO_VIEW_H
