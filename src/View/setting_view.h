#ifndef SETTING_VIEW_H
#define SETTING_VIEW_H

#include <QWidget>
#include<QLabel>
#include<QPushButton>
#include<QLineEdit>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include<QComboBox>
#include<QRadioButton>
#include<QFile>
#include<QString>
#include<QFileDialog>
#include<QCursor>
#include<QMessageBox>
#include<QDebug>
#include<QCameraDevice>
#include<QMediaDevices>
#include<QList>
#include<QMouseEvent>
#include<QEvent>
#include<QSignalBlocker>
#include<QSet>
#include<QShowEvent>
#include<QGroupBox>
#include<QApplication>
class Setting_view : public QWidget
{
    Q_OBJECT
public:
    explicit Setting_view(QWidget *parent = nullptr);
    void init_qss();
    void init_connect();
    void init_widget();
    void showWarningMessage(const QString&title,const QString&text);

    void Setupfirst_time(bool firstSetup);   //第一次进行系统配置操作

private:
    //标题栏，用于显示窗口标题和最大化、最小化、关闭窗口
    QPushButton *minButton;
    QPushButton *maxButton;
    QPushButton *closeButton;

    QLabel *Title_label;    //界面标题

    QLabel *Video_path;     //视频路径
    QLineEdit *video_edit;  //视屏路径显示
    QPushButton *Video_path_choose; //自定义选择视频存储的本地路径

    QLabel *Image_path;     //图片路径
    QLineEdit *image_edit;  //图片路径显示
    QPushButton *Image_path_choose;     //自定义选择图片存储的本地路径

    QLabel *time_interval;  //保存的时间间隔
    QComboBox *time_choose; //下拉框选择时间


    QLineEdit *Nameedit[4];     //自定义设备通道名称
    QComboBox *channel_choose[4];   //通道号选择

    QPushButton *Savebt;        //保存配置按钮
    QPushButton *Cancelbt;      //取消配置按钮

    bool is_dragging = false;       //窗口是否处于拖动标志
    QPoint drag_position;

    bool isfirstSetup=true;     //是否第一次进行配置操作的标志.用于区分第一次设置时点击"X"和在主界面进入setting_view点击"X"

protected:
    //重写，用于鼠标拖动窗口
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    //重写关闭窗口函数，用于应对不同情景下关闭setting_view界面
    void closeEvent(QCloseEvent *event);

    //重写显示函数：成功设置进入主界面再次进入设置页面
    void showEvent(QShowEvent *event);
signals:
public slots:
    void Saveconfig();
};

#endif // SETTING_VIEW_H
