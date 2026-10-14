
#include "monitor_view.h"

#include <QPixmap>
#include <QResizeEvent>

monitor_view::monitor_view(QWidget *parent)
    : QWidget{parent}
{
    init_qss();

    QHBoxLayout *main_layout = new QHBoxLayout(this);
    left_widget = new QWidget(this);
    right_widget = new QWidget(this);
    main_layout->addWidget(left_widget,1);
    main_layout->addWidget(right_widget,6);

    init_left_widget();
    init_right_widget();
    init_connect();

    // 持续监控：启动失败或读取失败的通道每五秒重试；页面隐藏不停止。
    auto *retryTimer = new QTimer(this);
    retryTimer->setInterval(5000);
    connect(retryTimer, &QTimer::timeout, this, [this]() {
        if (!monitoring_active) {
            return;
        }

        for (int i = 0; i < device_list->count() && i < channel_count; ++i)
            requestOpenCamera(i);
    });
    retryTimer->start();
}

void monitor_view::stopMonitoring()
{
    if(!monitor_control){
        return;
    }
    for(int index=0;index<channel_count;++index){
        requested_open[index]=false;
    }
    monitor_control->setDetectionMode(false);
    monitor_control->closeAllCamerasAndWait();
}

void monitor_view::flushCurrentRecordings()
{
    //刷新左侧视频列表
    if(monitor_control){
        monitor_control->flushCurrentRecordings();
    }
}

void monitor_view::init_qss()
{
    QFile file(":/qss/monitor_qss");
    if(file.open(QFile::ReadOnly)){
        setStyleSheet(file.readAll());
        file.close();
    }
}

void monitor_view::init_left_widget()
{
    //左侧设备列表
    QVBoxLayout *left_layout = new QVBoxLayout(left_widget);
    QLabel *camera_label = new QLabel("设备列表",left_widget);
    camera_label->setStyleSheet("color:black");
    camera_label->setAlignment(Qt::AlignCenter);    //文字居中
    camera_label->setObjectName("device");
    left_layout->addWidget(camera_label);

    device_list = new QListWidget(left_widget);
    left_layout->addWidget(device_list);
    refreshdevice();
}

void monitor_view::init_right_widget()
{
    QVBoxLayout *right_layout = new QVBoxLayout(right_widget);
    monitor_control = new monitorcontrol(this); //创建监控控制器对象，通过该对象构造监控线程与移动侦测线程

    status_label = new QLabel("正在启动监控通道...",right_widget);
    status_label->setAlignment(Qt::AlignCenter);

    //按钮水平布局-单通道模式-4通道模式-移动侦测模式
    QHBoxLayout *mode_layout = new QHBoxLayout;
    btn_v1 = new QPushButton("单通道",right_widget);
    btn_v4 = new QPushButton("四通道",right_widget);
    record_mode_button = new QPushButton("切换到移动侦测",right_widget);
    btn_v1->setCheckable(true);
    btn_v4->setCheckable(true);
    record_mode_button->setCheckable(true);
    btn_v1->setChecked(true);
    mode_layout->addWidget(btn_v1);
    mode_layout->addWidget(btn_v4);
    mode_layout->addWidget(record_mode_button);
    mode_layout->addStretch();

    //显示单个画面
    QWidget *video_container = new QWidget(right_widget);
    video_layout = new QStackedLayout(video_container);

    single_page = new QWidget(video_container);
    QVBoxLayout *single_layout = new QVBoxLayout(single_page);
    video_label_v1 = new QLabel(single_page);
    video_label_v1->setAlignment(Qt::AlignCenter);
    video_label_v1->setMinimumSize(320,240);
    video_label_v1->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    video_label_v1->setObjectName("videoLabel_1");
    single_layout->addWidget(video_label_v1);
    video_layout->addWidget(single_page);

    four_page = new QWidget(video_container);
    //网格布局显示4个画面
    QGridLayout *four_layout = new QGridLayout(four_page);
    four_layout->setSpacing(6);
    for(int index=0;index<channel_count;++index){
        QWidget *channel_page = new QWidget(four_page);
        QVBoxLayout *channel_layout = new QVBoxLayout(channel_page);
        channel_layout->setContentsMargins(0,0,0,0);
        video_name_v4[index] = new QLabel(channel_page);
        video_name_v4[index]->setAlignment(Qt::AlignCenter);
        video_name_v4[index]->setMinimumHeight(24);
        video_name_v4[index]->setStyleSheet("color:#222;font-weight:bold;");

        video_label_v4[index] = new QLabel(channel_page);
        video_label_v4[index]->setAlignment(Qt::AlignCenter);
        video_label_v4[index]->setMinimumSize(160,120);
        video_label_v4[index]->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        video_label_v4[index]->setStyleSheet("background:#111;color:#ddd;");
        channel_layout->addWidget(video_name_v4[index],0);
        channel_layout->addWidget(video_label_v4[index],1);
        four_layout->addWidget(channel_page,index/2,index%2);
    }
    video_layout->addWidget(four_page);


    snapshot_button = new QPushButton("手动截屏",right_widget);
    snapshot_button->setEnabled(false);

    //设置Object名称便于进行qss优化
    btn_v1->setObjectName("videoplay_1");
    btn_v4->setObjectName("videoplay_4");
    snapshot_button->setObjectName("shot_button");
    record_mode_button->setObjectName("record_button");


    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addWidget(snapshot_button);

    right_layout->addWidget(status_label,0);
    right_layout->addLayout(mode_layout,0);
    right_layout->addWidget(video_container,1);
    right_layout->addLayout(button_layout,0);

    updateFourChannelNames();
    refreshDisplayedImages();
}

void monitor_view::init_connect()
{
    connect(device_list,
            &QListWidget::itemClicked,
            this,
            [this](QListWidgetItem *item){
                //单击列表选项进行选中
                selectDeviceItem(item);
            });
    connect(device_list,
            &QListWidget::itemDoubleClicked,
            this,
            [this](QListWidgetItem *item){
                //双击列表选项进行选中并且转换为单画面
                selectDeviceItem(item);
                switch_to_v1();
            });

    connect(btn_v1,&QPushButton::clicked,this,&monitor_view::switch_to_v1); //点击按钮切换单通道
    connect(btn_v4,&QPushButton::clicked,this,&monitor_view::switch_to_v4); //点击按钮切换4通道
    //点击切换移动侦测模式
    connect(record_mode_button,
            &QPushButton::clicked,
            this,
            &monitor_view::toggleDetectionMode);
    //手动截屏
    connect(snapshot_button,
            &QPushButton::clicked,
            this,
            &monitor_view::takeSelectedSnapshot);

    //显示帧
    connect(monitor_control,
            &monitorcontrol::frameReady,
            this,
            [this](int viewIndex,const QImage &image){
                if(detection_mode){
                    return;
                }
                if(viewIndex<0||viewIndex>=channel_count){
                    return;
                }
                current_images[viewIndex]=image;
                showCurrentFrame(viewIndex,image);
            });
    //显示移动侦测帧
    connect(monitor_control,
            &monitorcontrol::detectionFrameReady,
            this,
            [this](int viewIndex,const QImage &image){
                if(!detection_mode
                    ||viewIndex<0||viewIndex>=channel_count){
                    return;
                }
                current_images[viewIndex]=image;
                showCurrentFrame(viewIndex,image);
            });
    connect(monitor_control,
            &monitorcontrol::detectionModeChanged,
            this,
            [this](bool enabled){
                detection_mode=enabled;
                const QSignalBlocker blocker(record_mode_button);
                record_mode_button->setChecked(enabled);
                record_mode_button->setText(enabled?"切换到普通监控":"切换到移动侦测");
                status_label->setText(enabled?"移动侦测录制中":"普通监控录制中");
            });
    connect(monitor_control,
            &monitorcontrol::cameraStateChanged,
            this,
            [this](int viewIndex,bool opened){
                if(viewIndex<0||viewIndex>=channel_count){
                    return;
                }
                camera_opened[viewIndex]=opened;
                if(opened) requested_open[viewIndex]=true;
                if(!opened){
                    requested_open[viewIndex]=false;
                    current_images[viewIndex]=QImage();
                    setPlaceholder(viewIndex,isVirtualCamera(viewIndex)?"示例视频未播放":"暂无监控画面");
                }
                updateControlButtons();
                if(opened){
                    if(four_channel_mode){
                        status_label->setText("四通道：真实摄像头监控，虚拟摄像头播放示例视频");
                    }else if(viewIndex==selected_view_index){
                        status_label->setText(QString(isVirtualCamera(viewIndex)?"%1：示例视频循环播放中":"%1已开启，正在实时录制").arg(cameraName(viewIndex)));
                    }
                }
            });
    //保存普通视频
    connect(monitor_control,
            &monitorcontrol::videoSaved,
            this,
            [this](const QString &path){
                qDebug()<<"录像文件已保存并写入video_t:"<<path;
                emit normalVideoSaved();
            });
    //保存移动侦测视频
    connect(monitor_control,
            &monitorcontrol::detectVideoSaved,
            this,
            [this](const QString &path){
                qDebug()<<"移动侦测录像已保存并写入detect_t:"<<path;
                emit detectionVideoSaved();
            });
    //保存手动截屏
    connect(monitor_control,
            &monitorcontrol::snapshotSaved,
            this,
            [this](const QString &path){
                status_label->setText(QString("截图已保存：%1").arg(path));
                emit snapshotSaved();
            });
    connect(monitor_control,
            &monitorcontrol::errorOccurred,
            this,
            [this](int viewIndex,const QString &message){
                if(viewIndex>=0&&viewIndex<channel_count){
                    requested_open[viewIndex]=false;
                    status_label->setText(QString("%1：%2") .arg(cameraName(viewIndex),message));
                }else{
                    status_label->setText(message);
                }
                updateControlButtons();
                qWarning()<<message;
            });
}

void monitor_view::switch_to_v1()
{
    four_channel_mode=false;
    video_layout->setCurrentWidget(single_page);
    btn_v1->setChecked(true);
    btn_v4->setChecked(false);
    refreshDisplayedImages();
    if(selected_view_index>=0 && selected_view_index<device_list->count()
        &&!requested_open[selected_view_index]){
        requestOpenCamera(selected_view_index);
    }
    updateControlButtons();
}

void monitor_view::switch_to_v4()
{
    four_channel_mode=true;
    video_layout->setCurrentWidget(four_page);
    btn_v1->setChecked(false);
    btn_v4->setChecked(true);
    updateFourChannelNames();
    refreshDisplayedImages();
    openAllAvailableCameras();
    updateControlButtons();
}

void monitor_view::toggleDetectionMode()
{
    detection_mode=record_mode_button->isChecked(); //标志由按钮是否点击确定
    monitor_control->setDetectionMode(detection_mode);
    status_label->setText(detection_mode?"正在切换到移动侦测...":"正在切换到普通监控...");    //文字显示
}

//切换为4通道画面，并开启所有真实及虚拟摄像头
void monitor_view::refreshdevice()
{
    if(!device_list){
        return;
    }

    int selectedChannel=-1;
    if(device_list->currentItem()){
        selectedChannel=device_list->currentItem()->data(Qt::UserRole).toInt();
    }

    const QSignalBlocker blocker(device_list);
    device_list->clear();
    int index=0;
    for(const camerainfo &camera : settinginfo::cameras){
        if(index>=channel_count){
            break;
        }
        QString name=camera.getChannelName().trimmed();
        if(name.isEmpty()){
            name=QString("通道%1").arg(camera.getChannelId());
        }

        QListWidgetItem *item=new QListWidgetItem(
            QIcon(":/image/blue_camera.png"),name);
        item->setData(Qt::UserRole,camera.getChannelId());
        item->setData(Qt::UserRole+1,camera.getCameraIndex());
        item->setToolTip(name);
        device_list->addItem(item);
        if(camera.getChannelId()==selectedChannel){
            device_list->setCurrentItem(item);
        }
        ++index;
    }

    if(!device_list->currentItem()&&device_list->count()>0){
        device_list->setCurrentRow(0);
    }
    if(device_list->currentItem()){
        selectDeviceItem(device_list->currentItem());
    }else{
        selected_view_index=0;
        selected_camera_id=-1;
        selected_device_index=-1;
    }
    updateFourChannelNames();
    refreshDisplayedImages();
    updateControlButtons();
}

int monitor_view::viewIndexForItem(const QListWidgetItem *item) const
{
    return item?device_list->row(const_cast<QListWidgetItem *>(item)):-1;
}

bool monitor_view::isVirtualCamera(int viewIndex) const
{
    if(!device_list||viewIndex<0||viewIndex>=device_list->count()){
        return true;
    }
    return device_list->item(viewIndex)->data(Qt::UserRole+1).toInt()<0;
}

QString monitor_view::cameraName(int viewIndex) const
{
    if(!device_list||viewIndex<0||viewIndex>=device_list->count()){
        return QString("通道%1").arg(viewIndex+1);
    }
    return device_list->item(viewIndex)->text();
}

void monitor_view::selectDeviceItem(QListWidgetItem *item)
{
    const int index=viewIndexForItem(item);
    if(index<0||index>=channel_count){
        return;
    }
    selected_view_index=index;
    selected_camera_id=item->data(Qt::UserRole).toInt();
    selected_device_index=item->data(Qt::UserRole+1).toInt();
    if(status_label&&four_channel_mode){
        status_label->setText(QString("已选择：%1，其他通道继续后台录制")
                                  .arg(cameraName(index)));
    }else if(status_label&&isVirtualCamera(index)){
        status_label->setText(QString("%1：示例视频通道") .arg(cameraName(index)));
    }else if(status_label){
        status_label->setText(QString("已选择：%1").arg(cameraName(index)));
    }
    refreshDisplayedImages();
    if(monitor_control && isVisible() && !four_channel_mode && isVirtualCamera(index)){
        requestOpenCamera(index);
    }
    updateControlButtons();
}

void monitor_view::requestOpenCamera(int viewIndex)
{
    if(viewIndex<0||viewIndex>=device_list->count()
        ||viewIndex>=channel_count||requested_open[viewIndex]){
        return;
    }
    const QListWidgetItem *item=device_list->item(viewIndex);
    const int channelId=item->data(Qt::UserRole).toInt();
    int deviceIndex=item->data(Qt::UserRole+1).toInt();
    if(isVirtualCamera(viewIndex)){
        int virtualIndex=0;
        for(int index=0;index<viewIndex;++index){
            if(isVirtualCamera(index)) ++virtualIndex;
        }
        deviceIndex=-virtualIndex-1;
    }
    if(channelId<=0){
        return;
    }
    requested_open[viewIndex]=true;
    monitor_control->openCamera(viewIndex,
                                deviceIndex,
                                channelId,
                                settinginfo::Time_interval,
                                settinginfo::Video_file_path,
                                settinginfo::Image_file_path);
    updateControlButtons();
}

void monitor_view::openAllAvailableCameras()
{
    status_label->setText("正在打开真实摄像头和示例视频通道...");
    for(int index=0;index<device_list->count()&&index<channel_count;++index){
        requestOpenCamera(index);
    }
}


void monitor_view::takeSelectedSnapshot()
{
    if(four_channel_mode){
        takeAllSnapshots();
        return;
    }
    monitor_control->takeSnapshot(selected_view_index);
}

void monitor_view::takeAllSnapshots()
{
    monitor_control->takeAllSnapshots();
}

void monitor_view::updateControlButtons()
{
    if (!snapshot_button || !device_list) return;
    if (four_channel_mode) {
        snapshot_button->setText("全部截屏");
        bool available = false;
        for (int i = 0; i < device_list->count() && i < channel_count; ++i)
            available |= camera_opened[i] && !isVirtualCamera(i);
        snapshot_button->setEnabled(available);
    } else {
        snapshot_button->setText("手动截屏");
        const bool valid = selected_view_index >= 0
                           && selected_view_index < device_list->count()
                           && selected_view_index < channel_count;
        snapshot_button->setEnabled(valid&& camera_opened[selected_view_index]&& !isVirtualCamera(selected_view_index));
    }
}

void monitor_view::updateFourChannelNames()
{
    if(!device_list){
        return;
    }
    for(int index=0;index<channel_count;++index){
        if(video_name_v4[index]){
            video_name_v4[index]->setText(cameraName(index));
        }
    }
}

void monitor_view::setPlaceholder(int viewIndex,const QString &text)
{
    if(viewIndex<0||viewIndex>=channel_count){
        return;
    }
    QLabel *label=video_label_v4[viewIndex];
    if(label){
        label->setPixmap(QPixmap());
        label->setText(text);
    }
    if(viewIndex==selected_view_index&&video_label_v1){
        video_label_v1->setPixmap(QPixmap());
        video_label_v1->setText(text);
    }
}

void monitor_view::showCurrentFrame(int viewIndex,const QImage &image)
{
    if(viewIndex<0||viewIndex>=channel_count||image.isNull()){
        return;
    }
    if(video_label_v4[viewIndex]){
        video_label_v4[viewIndex]->setPixmap(
            QPixmap::fromImage(image).scaled(video_label_v4[viewIndex]->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
    }
    if(!four_channel_mode&&viewIndex==selected_view_index&&video_label_v1){
        video_label_v1->setPixmap(
            QPixmap::fromImage(image).scaled(video_label_v1->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
    }
}

void monitor_view::refreshDisplayedImages()
{
    if(four_channel_mode){
        for(int index=0;index<channel_count;++index){
            if(current_images[index].isNull()){
                setPlaceholder(index,isVirtualCamera(index)?"示例视频未播放":"暂无监控画面");
            }else{
                showCurrentFrame(index,current_images[index]);
            }
        }
        return;
    }

    if(selected_view_index<0||selected_view_index>=channel_count){
        return;
    }
    if(current_images[selected_view_index].isNull()){
        setPlaceholder(selected_view_index,
                       isVirtualCamera(selected_view_index)?"示例视频未播放":"暂无监控画面");
    }else{
        showCurrentFrame(selected_view_index,current_images[selected_view_index]);
    }
}

void monitor_view::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!event->spontaneous()) {
        monitoring_active = true;
        refreshdevice();
        // 单画面只改变显示布局，四个通道均在后台工作。
        openAllAvailableCameras();
    }
}

void monitor_view::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshDisplayedImages();
}
