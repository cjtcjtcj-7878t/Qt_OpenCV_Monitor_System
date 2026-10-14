#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include <QWidget>
#include<QLabel>
#include<QFile>
#include<QLineEdit>
#include<QDebug>
#include<QPushButton>
#include<QComboBox>
#include<QString>
#include<QMessageBox>
#include<QSet>
#include<QStackedLayout>
#include<QTimer>
#include<QDateTime>
#include<QEvent>
#include<QButtonGroup>
#include<QIcon>
#include<QMouseEvent>
#include<QPoint>
#include<QStandardItem>
#include<QCloseEvent>
#include<QStandardItemModel>
#include<QMenu>
#include<QAction>
#include"monitor_view.h"
#include"normal_video_view.h"
#include"detect_video_view.h"
#include"image_video_view.h"
class main_view : public QWidget
{
    Q_OBJECT
public:
    explicit main_view(QWidget *parent = nullptr);
    void init_qss();        //qss优化界面
    void init_connect();        //信号与槽函数
    void init_timeShow();   //实时显示时间
    void init_topwidget();      //显示顶部导航栏
    void showUsername(const QString &name);        //显示登录的用户名称

    void showAccountMenu(const QString &username,const QDateTime &loginTime);   //显示帐户信息

    void showLoggedOut();       //显示"退出登录"后的界面
    void showWarningMessage(const QString &title,const QString &text);
private:

    QLabel *Real_time;      //显示实时时间
    QPushButton* minButton;     //窗口最小化按钮
    QPushButton* maxButton;     //窗口最大化按钮
    QPushButton* closeButton;       //关闭窗口

    QHBoxLayout *top_layout;    //顶部导航栏水平布局
    QStackedLayout *buttom_layout;  //主部堆栈布局进行不同按钮对应界面的切换
    monitor_view *Monitor;      //普通实时监控画面
    normal_video_view *Normal_video;    //普通视频回看画面
    detect_video_view *Detect_video;        //移动侦测视频回看画面
    image_video_view *Image_video;          //图片回看画面
    QLabel *Title_label;        //窗口标题


    //导航栏按钮
    QPushButton *Monitor_real_play;     //实时监控按钮
    QPushButton *Normal_video_replay;   //普通视频回放
    QPushButton *Detect_video_replay;   //移动侦测视频回放
    QPushButton *Image_video_replay;    //图片回放
    QPushButton *loag_research;     //日志查看
    QPushButton *Setting_btn;      //系统设置按钮

    QLabel *User_label;     //显示用户名
    QPushButton *login_btn;     //登录按钮
    QPushButton *loginout_btn;      //退出按钮
    bool is_dragging=false;     //是否处于拖动标志
    bool is_logined=false;      //是否处于成功登录状态
    QPoint drag_position;


signals:
    void accountButtonClicked();    //点击"未登录"发出信号
    void logoutRequested(); //退出登录的信号
public slots:
    void update_time();     //实时更新时间
protected:
    //用于鼠标拖动窗口
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    //重写窗口关闭事件
    void closeEvent(QCloseEvent *event);
};

#endif // MAIN_VIEW_H
