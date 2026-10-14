#ifndef NORMAL_VIDEO_VIEW_H
#define NORMAL_VIDEO_VIEW_H

#include <QWidget>
#include<QDateEdit>
#include<QCalendarWidget>
#include<QCalendar>
#include<QListWidgetItem>
#include<QListWidget>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QList>
#include<QFile>
#include<QDebug>
#include<QLabel>
#include<QPushButton>
#include<QComboBox>
#include<QSlider>
#include<QStackedLayout>
#include<QImage>
#include<QPixmap>
#include<QMouseEvent>
#include<QEvent>
#include<QResizeEvent>
#include<QShowEvent>
#include<QLocale>
#include<QString>
#include<QMessageBox>

class QFileSystemWatcher;

#include"../util/mysqlconnect.h"
#include"../util/dbconnectionpool.h"
#include"../Model/videoplayer.h"
#include"../Model/videomodel.h"
#include"../Model/video.h"
#include"../Model/settinginfo.h"
#include"../Service/videocontrol.h"
#include"../Service/videoplayercontrol.h"
class normal_video_view : public QWidget
{
    Q_OBJECT
public:
    explicit normal_video_view(QWidget *parent = nullptr);
    void init_qss();    //qss文件优化
    void init_left_widget();    //初始化左部分组件
    void init_right_widget();   //初始化右部分组件
    void init_connect();    //信号与槽
    void showWarningMessageBox(const QString &title,const QString &text);
private:
    void playVideo(const QString &videoPath);
    void resetPlayerUi(const QString &message="暂无播放");
    void updatePlayPauseButton(bool playing);
    void updatePlaybackPosition(qint64 position,qint64 duration);
    void showCurrentFrame(const QImage &image);
    void seekSliderByMouse(QMouseEvent *event);
    QString formatDuration(qint64 milliseconds) const;
    void updateVideoDirectoryWatcher();

    QWidget *left_widget;    //主部分的左组件-视频数据库分页查询
    QComboBox *channel_choose;      //选择通道号
    QListWidget *video_list;        //视频列表，用于显示视频文件
    QDateEdit *time_choose;     //视频列表选择时间
    QCalendarWidget *cal;         //视频列表选择日期控件
    QLabel *page_label;     //显示:当前页/总页数
    QPushButton *btn_last;  //向上翻页
    QPushButton *btn_next;  //向下翻页
    int current_page;       //当前的页数
    int total_page;     //总页数
    int page_size;      //每页展示的视频数

    bool ispage=false;    //是否进行上下翻页的标志

    QWidget *right_widget;      //主部分的右组件-视频播放
    QLabel *video_label=nullptr;
    QLabel *empty_label=nullptr;    //显示“无播放”
    QStackedLayout *player_stack=nullptr;   //堆栈布局，用于切换右侧视频正中的提示
    QSlider *progress_slider=nullptr;   //视频进度条
    QLabel *time_label=nullptr;     //视频的当前播放时间/总时间
    QPushButton *play_pause_button=nullptr; //暂停播放按钮
    QPushButton *snapshot_button=nullptr;   //手动截屏按钮
    QComboBox *speed_choose=nullptr;        //选择倍速的下拉框
    videoplayercontrol *player_control=nullptr;
    QImage current_image;   //MAT转为QIMAGE在QT界面上显示
    int current_video_id = 0;
    int current_camera_id = 0;
    QFileSystemWatcher *video_directory_watcher = nullptr;
    QString watched_video_directory;

signals:
public slots:
    void updateVideoList();     //根据时间选择以及通道选择更新左侧视频列表
    void refreshVideoList();
    void stopPlayback();

protected:
    bool eventFilter(QObject *watched,QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
};

#endif // NORMAL_VIDEO_VIEW_H
