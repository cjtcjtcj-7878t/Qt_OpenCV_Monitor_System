#ifndef MONITOR_VIEW_H
#define MONITOR_VIEW_H

#include <QDateTime>
#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include "../Service/monitorcontrol.h"
#include"../Model/settinginfo.h"
#include"../Service/videocontrol.h"
#include"../Service/videoplayercontrol.h"
#include"../Service/videoplayerworker.h"
class monitor_view : public QWidget
{
    Q_OBJECT

public:
    explicit monitor_view(QWidget *parent = nullptr);
    void stopMonitoring();
    void flushCurrentRecordings();
    void init_qss();    //调用qss文件优化
    void init_left_widget();    //初始化左组件
    void init_right_widget();       //初始化右组件
    void init_connect();    //信号与槽

private:
    static constexpr int channel_count = 4; //4通道画面

    QWidget *left_widget = nullptr; //左部分组件
    QWidget *right_widget = nullptr;    //右部分组件
    QListWidget *device_list = nullptr; //显示设备列表

    QPushButton *btn_v1 = nullptr;      //切换到单通道模式的按钮
    QPushButton *btn_v4 = nullptr;      //切换到4通道模式的按钮
    QPushButton *record_mode_button = nullptr;      //移动侦测与普通监控的按钮切换
    QWidget *single_page = nullptr;     //单通道模式显示
    QWidget *four_page = nullptr;       //4通道模式显示
    QLabel *video_label_v1 = nullptr;   //单通道
    QLabel *video_label_v4[channel_count] ={};  //4通道
    QLabel *video_name_v4[channel_count] = {};  //4通道的摄像头名称
    QStackedLayout *video_layout = nullptr;     //堆栈布局切换单通道与4通道

    QLabel *status_label = nullptr;
    QPushButton *snapshot_button = nullptr;     //手动截屏按钮
    monitorcontrol *monitor_control = nullptr;  //调用control方法

    QImage current_images[channel_count];
    bool camera_opened[channel_count] = {};
    bool requested_open[channel_count] = {};
    int selected_view_index = 0;
    int selected_camera_id = -1;       // settinginfo 中的 Channel_id
    int selected_device_index = -1;    // OpenCV 设备编号
    bool monitoring_active = false; // 退出时先停止自动重试
    bool four_channel_mode = false; //默认不开启4通道模式
    bool detection_mode = false;        //默认不开启移动侦测模式

    int viewIndexForItem(const QListWidgetItem *item) const;    //获取设备列表的行号，虚拟摄像头返回-1
    bool isVirtualCamera(int viewIndex) const;  //判断是否为虚拟摄像头
    QString cameraName(int viewIndex) const;
    void selectDeviceItem(QListWidgetItem *item);
    void requestOpenCamera(int viewIndex);
    void openAllAvailableCameras();
    void takeSelectedSnapshot();
    void takeAllSnapshots();
    void updateControlButtons();
    void updateFourChannelNames();
    void setPlaceholder(int viewIndex,const QString &text);
    void showCurrentFrame(int viewIndex,const QImage &image);
    void refreshDisplayedImages();
    void toggleDetectionMode();

public slots:
    void switch_to_v1();    //切换为单通道画面
    void switch_to_v4();    //切换为4通道画面
    void refreshdevice();

signals:
    void normalVideoSaved();    //保存普通视频信号
    void detectionVideoSaved(); //保存移动侦测视频信号
    void snapshotSaved();       //保存截图后通知图片回看页面刷新


protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MONITOR_VIEW_H
