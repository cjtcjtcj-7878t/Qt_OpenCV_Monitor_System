#include "main_view.h"
#include"../util/winmanage.h"
main_view::main_view(QWidget *parent)
    : QWidget{parent}
{
    this->resize(1200, 800);
    this->setMinimumSize(800, 550);
    this->setWindowFlag(Qt::FramelessWindowHint);   //无边框处理

    //窗口主布局
    QVBoxLayout *root_layout=new QVBoxLayout(this);
    root_layout->setContentsMargins(0,0,0,0);
    root_layout->setSpacing(0);

    // 顶部标题栏--------------------------------
    QWidget *titleBar=new QWidget(this);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(64);
    titleBar->setAttribute(Qt::WA_StyledBackground);

    QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(20, 0, 12, 0);
    titleBarLayout->setSpacing(12);

    //标题
    Title_label=new QLabel("智能监控系统",titleBar);
    Title_label->setObjectName("appTitle");

    //时间标签
    Real_time=new QLabel(titleBar);
    Real_time->setObjectName("timeLable");

    //登录按钮
    login_btn = new QPushButton("未登录", titleBar);
    login_btn->setObjectName("accountButton");
    login_btn->setIcon(QIcon(":/image/login.png"));
    login_btn->setIconSize(QSize(24, 24));
    login_btn->setFixedHeight(36);
    login_btn->setMinimumWidth(110);
    login_btn->setCursor(Qt::PointingHandCursor);
    login_btn->setToolTip("点击登录");

    //窗口最小化按钮
    minButton = new QPushButton("—", titleBar);
    minButton->setObjectName("minButton");
    minButton->setFixedSize(36, 36);

    //窗口关闭按钮
    closeButton=new QPushButton("X",titleBar);
    closeButton->setObjectName("closeButton");
    closeButton->setFixedSize(36,36);

    titleBarLayout->addWidget(Title_label);
    titleBarLayout->addStretch();
    titleBarLayout->addWidget(Real_time);
    titleBarLayout->addSpacing(8);
    titleBarLayout->addWidget(login_btn);
    titleBarLayout->addWidget(minButton);
    titleBarLayout->addWidget(closeButton);
    root_layout->addWidget(titleBar);

    //导航栏按钮
    QWidget *navBar = new QWidget(this);
    navBar->setObjectName("navBar");
    navBar->setFixedHeight(60);
    navBar->setAttribute(Qt::WA_StyledBackground);

    top_layout = new QHBoxLayout(navBar);
    top_layout->setContentsMargins(20, 10, 20, 10);
    top_layout->setSpacing(8);

    init_topwidget();
    root_layout->addWidget(navBar);

    // 内容区域：将监控、回放等页面放入此布局
    QWidget *content = new QWidget(this);
    content->setObjectName("contentArea");

    Monitor=new monitor_view; //监控画面
    Normal_video=new normal_video_view;     //普通视频回看
    Detect_video=new detect_video_view;     //移动侦测回看
    Image_video=new image_video_view;       //图片回看
    buttom_layout = new QStackedLayout(content);    //内容展示部分使用堆叠布局切换不同内容
    buttom_layout->setContentsMargins(20, 20, 20, 20);
    //堆叠布局加入组件
    buttom_layout->addWidget(Monitor);  //索引为0
    buttom_layout->addWidget(Normal_video); //索引为1
    buttom_layout->addWidget(Detect_video); //索引为2
    buttom_layout->addWidget(Image_video); //索引为3
    root_layout->addWidget(content,1);

    //显示时间在右上角
    titleBarLayout->addSpacing(600);
    titleBarLayout->addWidget(Real_time);

    //登录按钮
    // QWidget *btn_login_box=new QWidget;
    // QVBoxLayout *btn_login_layout=new QVBoxLayout;
    // btn_login_box->setLayout(btn_login_layout);
    // btn_login_=new QPushButton("");
    // btn_login_->setIcon(QIcon(":/Image/登录.png"));
    // btn_login_->setIconSize(QSize(40,50));
    // btn_login_layout->addWidget(btn_login_);

    // //显示登录的用户名
    // User_label=new QLabel("未登录");
    // User_label->setStyleSheet("color:white");
    // User_label->setAlignment(Qt::AlignCenter);
    // login_btn_layout->addWidget(User_label);
    // login_btn_layout->addStretch();
    // titleBarLayout->addWidget(login_btn_widget);


    // //系统设置按钮
    // Setting_btn=new QPushButton("系统设置");
    // Setting_btn->setFixedSize(100,60);
    // titleBarLayout->addWidget(Setting_btn);
    this->init_timeShow();  //实时显示时间
    this->init_connect();
    this->init_qss();

}

void main_view::init_qss()
{
    QFile file(":/qss/main_qss");
    if(file.open(QFile::ReadOnly)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void main_view::init_connect()
{
    //最小化窗口
    connect(minButton,&QPushButton::clicked,this,&QWidget::showMinimized);

    //关闭窗口
    connect(closeButton,&QPushButton::clicked,this,[this](){
        this->close();
    });

    //发送accountButtonClicked()信号
    connect(login_btn,&QPushButton::clicked,this,[this](){
        emit accountButtonClicked();
        //点击账户名后发出点击 信号
    });

    //点击系统设置按钮
    connect(Setting_btn,&QPushButton::clicked,this,[this](){
        winmanage *manager = winmanage::Getinstance();

        // 明确标记为从主界面打开设置
        manager->setting_widget.Setupfirst_time(false);
        manager->setting_widget.show();
        this->hide();
    });

    //切换到监控
    connect(Monitor_real_play,&QPushButton::clicked,this,[this](){
        Normal_video->stopPlayback();
        Detect_video->stopPlayback();
        Image_video->stopPlayback();
        buttom_layout->setCurrentIndex(0);
    });

    //切换到普通视频回看
    connect(Normal_video_replay,&QPushButton::clicked,this,[this](){
        if(!is_logined){
            showWarningMessage("提示","请先登录后再使用普通视频回放功能!");
            return;
        }
        Detect_video->stopPlayback();
        Image_video->stopPlayback();
        buttom_layout->setCurrentIndex(1);
    });

    //切换到移动侦测回看
    connect(Detect_video_replay,&QPushButton::clicked,this,[this](){
        if(!is_logined){
            showWarningMessage("提示","请先登录后再使用移动侦测回放功能!");
            return;
        }

        Normal_video->stopPlayback();
        Image_video->stopPlayback();
        buttom_layout->setCurrentIndex(2);
    });

    //切换到图片回看。图片页面同样要求登录，防止未登录状态读取回看资料。
    connect(Image_video_replay,&QPushButton::clicked,this,[this](){
        if(!is_logined){
            showWarningMessage("提示","请先登录后再使用图片回看功能!");
            return;
        }
        Normal_video->stopPlayback();
        Detect_video->stopPlayback();
        buttom_layout->setCurrentIndex(3);
    });

    //点击日志查看按钮
    connect(loag_research,&QPushButton::clicked,this,[this](){
        showWarningMessage("提示","该功能尚在开发");
        return;
        Monitor->flushCurrentRecordings();
    });

    // 录像分段完成后，回放页面会立即重新读取数据库和保存目录。
    connect(Monitor,
            &monitor_view::normalVideoSaved,
            Normal_video,
            &normal_video_view::refreshVideoList);
    connect(Monitor,
            &monitor_view::detectionVideoSaved,
            Detect_video,
            &detect_video_view::refreshVideoList);
    connect(Monitor,
            &monitor_view::normalVideoSaved,
            Image_video,
            &image_video_view::refreshImageList);
    connect(Monitor,
            &monitor_view::detectionVideoSaved,
            Image_video,
            &image_video_view::refreshImageList);
    connect(Monitor,
            &monitor_view::snapshotSaved,
            Image_video,
            &image_video_view::refreshImageList);


}

void main_view::init_timeShow()
{
    //初始化定时器
    QTimer *time_str=new QTimer;

    //设置定时器间隔为1秒
    time_str->setInterval(1000);    //单位为ms
    //连接信号与槽函数
    connect(time_str,&QTimer::timeout,this,&main_view::update_time);
    time_str->start();
    update_time();
}

void main_view::init_topwidget()
{

    //导航栏标题
    Title_label=new QLabel("智能监控系统");
    Title_label->setAlignment(Qt::AlignCenter);
    this->top_layout->addWidget(Title_label);

    //导航栏按钮
    Monitor_real_play=new QPushButton("监控主页面");
    Monitor_real_play->setObjectName("navButton");
    this->top_layout->addWidget(Monitor_real_play);

    Normal_video_replay=new QPushButton("普通视频回放");
    Normal_video_replay->setObjectName("navButton");
  //默认不允许使用普通视频回看功能，登录后才可以使用
    this->top_layout->addWidget(Normal_video_replay);

    Detect_video_replay=new QPushButton("移动侦测回放");
    Detect_video_replay->setObjectName("navButton");
   //默认不允许使用移动侦测回看功能，登录后才可以使用
    this->top_layout->addWidget(Detect_video_replay);

    Image_video_replay=new QPushButton("图片回看");
    Image_video_replay->setObjectName("navButton");
    this->top_layout->addWidget(Image_video_replay);

    loag_research=new QPushButton("日志查看");
    loag_research->setObjectName("navButton");
    this->top_layout->addWidget(loag_research);

   //系统设置按钮
    Setting_btn=new QPushButton("系统设置");
    Setting_btn->setObjectName("settingsButton");
    this->top_layout->addWidget(Setting_btn);




}

void main_view::showUsername(const QString &name)
{
    //登录成功后显示用户名
    is_logined=true;
    login_btn->setText(name);
    login_btn->setToolTip("查看登录信息或退出登录");

    //允许使用普通视频回看功能与移动侦测回看功能
    Normal_video_replay->setEnabled(true);
    Detect_video_replay->setEnabled(true);
    Image_video_replay->setEnabled(true);

    show();

}

void main_view::showAccountMenu(const QString &username, const QDateTime &loginTime)
{
    QMenu menu(this);
    //登录成功后显示账户相关信息
    menu.addAction("账号：" + username)->setEnabled(false);    //显示账户名

    menu.addAction("登录时间：" + loginTime.toString("yyyy-MM-dd HH:mm:ss"))->setEnabled(false); //显示登录时间

    menu.addSeparator();

    QAction *logoutAction = menu.addAction("退出登录");

    // 在账号按钮下方显示菜单
    QAction *selected = menu.exec(login_btn->mapToGlobal(QPoint(0, login_btn->height())));

    //登录状态下点击"退出登录"
    if (selected == logoutAction) {
        //确认信息
        QMessageBox::StandardButton result=QMessageBox::question(this,"提示","确认退出登录?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
        if(result==QMessageBox::Yes){
        emit logoutRequested();
        }
    }
}

void main_view::showLoggedOut()
{
    //成功退出登录后，main_view显示相应的画面
    is_logined=false;
    //禁止使用普通回看功能与移动侦测回看功能
   // Normal_video_replay->setEnabled(false);
   // Detect_video_replay->setEnabled(false);
    //功能按钮可点击，但是弹出登录提示
    Normal_video->stopPlayback();
    Detect_video->stopPlayback();
    Image_video->stopPlayback();
    buttom_layout->setCurrentIndex(0);  //切换到监控页面
    login_btn->setText("未登录");
    login_btn->setToolTip("点击登录");

}

void main_view::showWarningMessage(const QString &title, const QString &text)
{
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::NoIcon);
    msg.setWindowTitle(title);
    msg.setText(text);
    msg.setStandardButtons(QMessageBox::Ok);

    // 找到 QMessageBox 内部显示文本的 QLabel
    QLabel *label = msg.findChild<QLabel*>("qt_msgbox_label");
    if(label)
    {
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setMinimumWidth(280);
    }

    msg.exec();
}

void main_view::update_time()
{
    //实时显示时间
    //获取格式化的时间
    QDateTime currentTime=QDateTime::currentDateTime();
    QString now=currentTime.toString("yyyy-MM-dd HH:mm:ss");
    //转换成年-月-日 时：分：秒形式
    Real_time->setText(now);
}

void main_view::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        is_dragging=true;
        drag_position=event->globalPosition().toPoint()-this->frameGeometry().topLeft();
        event->accept();
    }
}

void main_view::mouseReleaseEvent(QMouseEvent *event)
{
    is_dragging=false;
    event->accept();
}

void main_view::mouseMoveEvent(QMouseEvent *event)
{
    if(is_dragging&&(event->buttons()&Qt::LeftButton)){
        this->move(event->globalPosition().toPoint()-drag_position);
        event->accept();
    }
}

void main_view::closeEvent(QCloseEvent *event)
{
    //在主界面点击"X"按钮
    QMessageBox::StandardButton result=QMessageBox::question(this,"提示","是否确认退出监控系统?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    if(result==QMessageBox::Yes){
        //确认退出系统
        Monitor->stopMonitoring();
        event->accept();
        //接收关闭窗口事件
        QApplication::quit();
        //直接退出程序
    }else{
        //忽略关闭事件
        event->ignore();
    }
}
