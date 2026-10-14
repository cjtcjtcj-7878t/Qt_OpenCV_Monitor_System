#include "setting_view.h"
#include"../util/winmanage.h"
#include "../Service/cameracontrol.h"

Setting_view::Setting_view(QWidget *parent)
    : QWidget{parent}
{
    this->setObjectName("Settingwidget");

    this->init_qss();
    this->init_widget();
    this->init_connect();
}

void Setting_view::init_qss()
{
    //调用qss优化界面
    QFile file(":/qss/setting_qss");
    if(file.open(QFile::ReadOnly)){
        this->setStyleSheet(file.readAll());
        //qDebug()<<"qss加载成功";
        file.close();
    }

}

void Setting_view::init_connect()
{
    connect(Video_path_choose,&QPushButton::clicked,this,[this](){
        //弹出文件资源管理器选择本地存储路径
        QString path_video=QFileDialog::getExistingDirectory(this,tr("请选择存储路径"),"./");

        if(!path_video.isEmpty()){//选择了路径
            this->video_edit->setText(path_video);
            //显示路径
            this->video_edit->setCursorPosition(0);
            //强制显示路径内容
            this->video_edit->setToolTip(path_video);
            //鼠标悬停时显示完整的路径
        }else{
            qDebug()<<"用户取消选择视频路径";
        }
    });     //视频保存路径

    connect(Image_path_choose,&QPushButton::clicked,this,[this](){
        //弹出文件资源管理器选择本地存储路径
        QString path_image=QFileDialog::getExistingDirectory(this,tr("请选择存储路径"),"./");

        if(!path_image.isEmpty()){//选择了路径
            this->image_edit->setText(path_image);
            //显示路径
            this->image_edit->setCursorPosition(0);
            //强制显示路径内容
            this->image_edit->setToolTip(path_image);
            //鼠标悬停时显示完整的路径
        }else{
            qDebug()<<"用户取消选择图片路径";
        }
    });     //图片保存路径

    //最小化窗口
    connect(minButton,&QPushButton::clicked,this,[this](){
        this->showMinimized();
    });

    //最大化窗口
    connect(maxButton,&QPushButton::clicked,this,[this](){
        if(this->isMaximized()){
            this->showNormal();
        }else{
            this->showMaximized();
        }
    });

    //关闭窗口"X"与"取消"按钮的关闭逻辑
    connect(closeButton,&QPushButton::clicked,this,[this](){
        this->close();
    });
    connect(Cancelbt,&QPushButton::clicked,this,[this](){
        this->close();
    });

    //点击保存按钮
    connect(Savebt,&QPushButton::clicked,this,&Setting_view::Saveconfig);

}

void Setting_view::init_widget()
{

    //窗口标题设置
    QWidget *title_bar=new QWidget(this);
    QLabel *window_title =new QLabel("智能监控系统设置",title_bar);

    //窗口标题
    minButton =new QPushButton("—",title_bar);
    //窗口最小化
    maxButton =new QPushButton("[]",title_bar);
    //窗口最大化
    closeButton =new QPushButton("x",title_bar);
    //关闭窗口


    QHBoxLayout *window_title_layout =new QHBoxLayout(title_bar);

    window_title_layout->setContentsMargins(15, 0, 0, 0);
    window_title_layout->setSpacing(0);

    window_title_layout->addWidget(window_title);

    window_title_layout->addStretch();

    window_title_layout->addWidget(minButton);
    window_title_layout->addWidget(maxButton);
    window_title_layout->addWidget(closeButton);



    //界面主布局-垂直布局


    this->setFixedSize(800,600);    //界面大小
    QVBoxLayout *main_layout=new QVBoxLayout;
    this->setLayout(main_layout);
    this->setWindowFlag(Qt::FramelessWindowHint);   //界面窗口变成无边框样式，但失去拖动、关闭功能、调整大小功能
    main_layout->setSpacing(20);    //界面组件间隔
    this->setContentsMargins(100,10,100,10);
    main_layout->insertWidget(0,title_bar);

    main_layout->addStretch(1);
    //界面标题设置
    QHBoxLayout *title_layout=new QHBoxLayout;  //标题垂直居中显示
    main_layout->addStretch(1);
    Title_label=new QLabel("系统设置");
    QLabel *subTitle = new QLabel("配置存储位置、保存间隔以及摄像头通道");
    subTitle->setObjectName("subTitle");
    Title_label->setAlignment(Qt::AlignCenter);
    title_layout->addWidget(Title_label);
    title_layout->addWidget(subTitle);
    main_layout->addLayout(title_layout);
    main_layout->addStretch(1);


    //视频保存路径设置
    QHBoxLayout *video_path_layout=new QHBoxLayout;
    Video_path = new QLabel("视频保存路径:");
    Video_path->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    video_edit = new QLineEdit;
    Video_path_choose = new QPushButton("浏览");
    video_edit->setReadOnly(true);
    video_path_layout->addWidget(Video_path,2);
    video_path_layout->addWidget(video_edit,8);
    video_path_layout->addWidget(Video_path_choose,1);
    main_layout->addLayout(video_path_layout);


    video_edit->setText(settinginfo::Video_file_path);

    //图片保存路径设置
    QHBoxLayout *image_path_layout=new QHBoxLayout;
    Image_path = new QLabel("图片保存路径:");
    Image_path->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    image_edit = new QLineEdit;
    Image_path_choose = new QPushButton("浏览");
    image_edit->setReadOnly(true);
    image_path_layout->addWidget(Image_path,2);
    image_path_layout->addWidget(image_edit,8);
    image_path_layout->addWidget(Image_path_choose,1);
    main_layout->addLayout(image_path_layout);


    image_edit->setText(settinginfo::Image_file_path);

    //保存的时间间隔设置
    QHBoxLayout *time_interval_layout=new QHBoxLayout;
    time_interval = new QLabel("保存时间间隔:");
    time_interval->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    time_choose = new QComboBox;
    time_choose->addItem("15秒", 15);
    time_choose->addItem("30秒", 30);
    time_choose->addItem("45秒", 45);
    time_choose->addItem("60秒", 60);
    time_interval_layout->addWidget(time_interval,2);
    time_interval_layout->addWidget(time_choose,2);
    time_interval_layout->addStretch(8);
    main_layout->addLayout(time_interval_layout);

    int index = time_choose->findData(settinginfo::Time_interval);

    // 首次配置时默认30秒
    if (index < 0) {
        index = time_choose->findData(30);
    }
    time_choose->setCurrentIndex(index);
    //摄像头配置
    //自动获取硬件拥有的摄像头名称
    // QList<QCameraDevice> cameras=QMediaDevices::videoInputs();

    // for(int i=0;i<4;i++){
    //     //构建实例
    //     Camera_name[i]=new QLabel;
    //     Nameedit[i]=new QLineEdit;
    //     channel_choose[i]=new QComboBox;

    //     //存在摄像头
    //     if(i<cameras.size()){
    //         //获取名称
    //         QString CameraName=cameras[i].description();
    //         Camera_name[i]->setText(CameraName);
    //     }else{
    //         //不存在摄像头
    //         Camera_name[i]->setText("virtual camera");
    //     }


    //     //通道选择
    //     channel_choose[i]->addItem("通道1");
    //     channel_choose[i]->addItem("通道2");
    //     channel_choose[i]->addItem("通道3");
    //     channel_choose[i]->addItem("通道4");

    //     //布局
    //     QHBoxLayout *camera_layout=new QHBoxLayout;
    //     camera_layout->addWidget(Camera_name[i],2);
    //     camera_layout->addWidget(Nameedit[i],8);
    //     camera_layout->addWidget(channel_choose[i],2);
    //     main_layout->addLayout(camera_layout);
    // }
    //摄像头配置
    //qDebug()<<"检测到摄像头数量:"<<settinginfo::cameras.size();

    if(settinginfo::cameras.size()<4){
        qDebug()<<"摄像头数量不足4个，重新初始化";
        settinginfo::initCamera();
    }
    for(int i=0;i<4;i++){
        QHBoxLayout *h_layout=new QHBoxLayout;
        QLabel *camera_label=new QLabel(settinginfo::cameras[i].getCameraName());
        camera_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        Nameedit[i]=new QLineEdit(settinginfo::cameras[i].getChannelName());
        channel_choose[i]=new QComboBox();

        channel_choose[i]->addItem("通道1",1);
        channel_choose[i]->addItem("通道2",2);
        channel_choose[i]->addItem("通道3",3);
        channel_choose[i]->addItem("通道4",4);
        channel_choose[i]->setCurrentIndex(settinginfo::cameras[i].getChannelId()-1);

        h_layout->addWidget(camera_label,3);
        h_layout->addWidget(Nameedit[i],5);
        h_layout->addWidget(channel_choose[i],2);
        main_layout->addLayout(h_layout);

    }

    //保存、取消按钮

    QHBoxLayout *btn_layout=new QHBoxLayout;
    Savebt=new QPushButton("保存");
    Cancelbt=new QPushButton("取消");
    btn_layout->addStretch(1);
    btn_layout->addWidget(Savebt);
    btn_layout->addStretch(1);
    btn_layout->addWidget(Cancelbt);
    btn_layout->addStretch(1);
    main_layout->addLayout(btn_layout);
    Title_label->setObjectName("titleLabel");
    Video_path_choose->setObjectName("browseButton");
    Image_path_choose->setObjectName("browseButton");
    Savebt->setObjectName("saveButton");
    Cancelbt->setObjectName("cancelButton");
    title_bar->setObjectName("titleBar");
    window_title->setObjectName("windowTitle");
    minButton->setObjectName("minButton");
    maxButton->setObjectName("maxButton");
    closeButton->setObjectName("closeButton");
    main_layout->addStretch(1);

}

void Setting_view::showWarningMessage(const QString &title, const QString &text)
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

void Setting_view::Setupfirst_time(bool firstSetup)
{
    this->isfirstSetup=firstSetup;
}


void Setting_view::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        is_dragging=true;
        drag_position=event->globalPosition().toPoint()-this->frameGeometry().topLeft();
        event->accept();
    }
}

void Setting_view::mouseMoveEvent(QMouseEvent *event)
{
    if(is_dragging&&(event->buttons()&Qt::LeftButton)){
        this->move(event->globalPosition().toPoint()-drag_position);
        event->accept();
    }
}

void Setting_view::mouseReleaseEvent(QMouseEvent *event)
{
    is_dragging=false;
    event->accept();
}

void Setting_view::closeEvent(QCloseEvent *event)
{
    if (isfirstSetup) {
        // 首次配置：询问是否退出系统
        QMessageBox::StandardButton result=QMessageBox::question
            (this,"提示","确认退出系统？",QMessageBox::Yes | QMessageBox::No,QMessageBox::No);

        if (result == QMessageBox::Yes) {
            event->accept();       // 允许关闭窗口
            QApplication::quit();  // 退出系统
        }
        else
        {
            event->ignore();       // 拒绝关闭，继续停留在设置界面
        }

        return;
    }

    // 从主界面打开的设置：返回主界面
    winmanage::Getinstance()->main_widget.show();
    event->accept();
}

void Setting_view::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 窗口系统引起的显示事件不重置正在编辑的内容
    if (event->spontaneous()) {
        return;
    }

    // settinginfo保存的是已加载或已成功保存的配置
    video_edit->setText(settinginfo::Video_file_path);
    video_edit->setToolTip(settinginfo::Video_file_path);
    video_edit->setCursorPosition(0);

    image_edit->setText(settinginfo::Image_file_path);
    image_edit->setToolTip(settinginfo::Image_file_path);
    image_edit->setCursorPosition(0);

    int timeIndex = time_choose->findData(settinginfo::Time_interval);

    if (timeIndex < 0) {
        timeIndex = time_choose->findData(30);
    }

    time_choose->setCurrentIndex(timeIndex);

    for (int i = 0; i < 4 && i < settinginfo::cameras.size(); i++) {
        const camerainfo &camera = settinginfo::cameras.at(i);

        const QSignalBlocker nameBlocker(Nameedit[i]);
        const QSignalBlocker channelBlocker(channel_choose[i]);

        Nameedit[i]->setText(camera.getChannelName());

        channel_choose[i]->setCurrentIndex(channel_choose[i]->findData(camera.getChannelId()));
    }
}

void Setting_view::Saveconfig()
{

    //检查视频保存路径
    if(video_edit->text().trimmed().isEmpty()){
        showWarningMessage("配置错误","视频路径未配置!请重新选择");
        return;
    }

    //检查图片保存路径
    if(image_edit->text().trimmed().isEmpty()){
        showWarningMessage("配置错误","图片路径未配置!请重新选择");
        return;
    }

    //检查通道是否重复
    QSet<int> usedChannel;
    for(int i=0;i<4;i++){
        int channelId=channel_choose[i]->currentData().toInt();
        if(usedChannel.contains(channelId)){
            showWarningMessage("配置错误","通道选择有重复!请重新选择");
            return;
        }
        if(Nameedit[i]->text().trimmed().isEmpty()){
            showWarningMessage("配置错误","通道名称不能为空!");
            return;
        }
        usedChannel.insert(channelId);
    }

    QMessageBox::StandardButton result=QMessageBox::question(this,"确认保存配置","是否确认保存以上信息?");
    if(result==QMessageBox::Yes){

        //上一次的配置
        const QString oldvideo_path=settinginfo::Video_file_path;
        const QString oldimage_path=settinginfo::Image_file_path;
        const int oldtime_interval=settinginfo::Time_interval;
        const QList<camerainfo> oldcamera_list=settinginfo::cameras;

        settinginfo::Video_file_path=video_edit->text();        //视频保存路径赋值到settinginfo
        settinginfo::Image_file_path=image_edit->text();        //图片保存路径赋值到settinginfo
        settinginfo::Time_interval=time_choose->currentData().toInt();      //保存时间间隔赋值到settinginfo

        for(int i=0;i<4;i++){
            settinginfo::cameras[i].setChannelName(Nameedit[i]->text().trimmed());
            settinginfo::cameras[i].setChannelId(channel_choose[i]->currentData().toInt());
        }
        //调用SaveTofile函数

        QString errorMessage;
        if(!settinginfo::SaveTofile(&errorMessage)){
            settinginfo::Video_file_path=oldvideo_path;
            settinginfo::Image_file_path=oldimage_path;
            settinginfo::Time_interval=oldtime_interval;
            settinginfo::cameras=oldcamera_list;

            showWarningMessage("保存失败","配置未保存，请重试"+settinginfo::configPath()+"\n"+errorMessage);
            return;
        }

        cameracontrol cameraControl;
        if(!cameraControl.syncCameras(settinginfo::cameras,errorMessage)){
            settinginfo::Video_file_path=oldvideo_path;
            settinginfo::Image_file_path=oldimage_path;
            settinginfo::Time_interval=oldtime_interval;
            settinginfo::cameras=oldcamera_list;

            QString rollbackFileError;
            settinginfo::SaveTofile(&rollbackFileError);
            showWarningMessage("保存失败",QString("camera_t同步失败，配置已回滚。\n%1").arg(errorMessage));
            return;
        }

        QMessageBox::information(this,"提示","配置信息保存成功");

        //完成首次配置
        Setupfirst_time(false);
        //隐藏设置页面，显示主页面
        winmanage::Getinstance()->main_widget.show();
        this->hide();
    }


}
