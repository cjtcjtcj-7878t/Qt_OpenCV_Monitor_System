#include "detect_video_view.h"

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMouseEvent>
#include <QPixmap>
#include <QSignalBlocker>

detect_video_view::detect_video_view(QWidget *parent)
    : QWidget(parent)
{
    // 左侧是setting_view中的Video_file_path筛选，右侧是 OpenCV 视频播放控件。
    QHBoxLayout *main_layout=new QHBoxLayout(this);
    left_widget=new QWidget(this);
    right_widget=new QWidget(this);
    main_layout->addWidget(left_widget,2);
    main_layout->addWidget(right_widget,6);
    init_qss();
    init_left_widget();
    init_right_widget();
    init_connect();
    updateVideoList();
}

void detect_video_view::init_qss()
{
    //调用qss文件优化界面
    QFile file(":/qss/detect_video_replay");
    if(file.open(QFile::ReadOnly)){
        setStyleSheet(file.readAll());
        file.close();
    }
}

void detect_video_view::init_left_widget()
{
    //左组件主布局--垂直布局
    QVBoxLayout *left_layout=new QVBoxLayout(left_widget);
    //选择时间和通道布局--水平布局
    QHBoxLayout *select_layout=new QHBoxLayout;

    //选择日期控件
    time_choose=new QDateEdit(left_widget);
    time_choose->setDisplayFormat("yyyy年MM月dd日");
    time_choose->setDate(QDate::currentDate());
    time_choose->setCalendarPopup(true);
    time_choose->setLocale(QLocale(QLocale::Chinese));
    calendar=time_choose->calendarWidget();
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    select_layout->addWidget(time_choose,6);

    //选择通道控件
    channel_choose=new QComboBox(left_widget);
    channel_choose->addItem("所有通道",0);
    for(int channel=1;channel<=4;++channel){
        channel_choose->addItem(QString("通道%1").arg(channel),channel);
    }
    select_layout->addWidget(channel_choose,4);
    left_layout->addLayout(select_layout,1);

    video_list=new QListWidget(left_widget);
    left_layout->addWidget(video_list,8);

    QWidget *page_widget=new QWidget(left_widget);
    QHBoxLayout *page_layout=new QHBoxLayout(page_widget);
    btn_last=new QPushButton("<<",page_widget);
    btn_next=new QPushButton(">>",page_widget);
    page_label=new QLabel("1/1",page_widget);
    page_label->setAlignment(Qt::AlignCenter);
    page_layout->addWidget(btn_last);
    page_layout->addWidget(page_label);
    page_layout->addWidget(btn_next);
    left_layout->addWidget(page_widget,1);
}

void detect_video_view::init_right_widget()
{
    QVBoxLayout *right_layout=new QVBoxLayout(right_widget);
    player_control=new videoplayercontrol(this);
    video_label=new QLabel(right_widget);
    empty_label=new QLabel("暂无播放",right_widget);
    player_stack=new QStackedLayout;
    empty_label->setAlignment(Qt::AlignCenter);
    empty_label->setStyleSheet("background:#111;color:#ddd;font-size:24px;");
    video_label->setAlignment(Qt::AlignCenter);
    video_label->setStyleSheet("background:#111;color:#ddd;");
    player_stack->addWidget(empty_label);
    player_stack->addWidget(video_label);
    player_stack->setCurrentWidget(empty_label);
    right_layout->addLayout(player_stack,8);

    QHBoxLayout *control_layout=new QHBoxLayout;
    play_pause_button=new QPushButton("播放",right_widget);
    play_pause_button->setEnabled(false);
    control_layout->addWidget(play_pause_button);
    progress_slider=new QSlider(Qt::Horizontal,right_widget);
    progress_slider->setRange(0,0);
    progress_slider->installEventFilter(this);
    control_layout->addWidget(progress_slider,8);
    time_label=new QLabel("00:00/00:00",right_widget);
    time_label->setMinimumWidth(100);
    time_label->setAlignment(Qt::AlignCenter);
    control_layout->addWidget(time_label);
    speed_choose=new QComboBox(right_widget);
    speed_choose->addItem("1倍速",1.0);
    speed_choose->addItem("1.5倍速",1.5);
    speed_choose->addItem("2倍速",2.0);
    speed_choose->addItem("4倍速",4.0);
    control_layout->addWidget(speed_choose);
    snapshot_button=new QPushButton("手动截屏",right_widget);
    snapshot_button->setEnabled(false);
    control_layout->addWidget(snapshot_button);
    right_layout->addLayout(control_layout,1);
}

void detect_video_view::init_connect()
{
    connect(btn_last,&QPushButton::clicked,this,[this](){
        if(current_page>1){
            --current_page;
            changing_page=true;
            updateVideoList();
        }else{
            showWarningMessageBox("提示","已是第一页!");
        }
    });
    connect(btn_next,&QPushButton::clicked,this,[this](){
        if(current_page<total_page){
            ++current_page;
            changing_page=true;
            updateVideoList();
        }else{
            showWarningMessageBox("提示","已是最后一页!");
        }
    });
    connect(time_choose,&QDateEdit::dateChanged,this,[this](){
        current_page=1;
        updateVideoList();
    });
    connect(channel_choose,&QComboBox::currentIndexChanged,this,[this](){
        current_page=1;
        updateVideoList();
    });
    connect(video_list,&QListWidget::itemDoubleClicked,this,[this](const QListWidgetItem *item){
        current_video_id=item->data(Qt::UserRole+1).toInt();
        current_camera_id=item->data(Qt::UserRole+3).toInt();
        playVideo(item->data(Qt::UserRole).toString());
    });
    connect(play_pause_button,&QPushButton::clicked,
            player_control,&videoplayercontrol::togglePlayback);
    connect(player_control,&videoplayercontrol::frameReady,this,[this](const QImage &image){
        current_image=image;
        player_stack->setCurrentWidget(video_label);
        showCurrentFrame(image);
    });
    connect(player_control,&videoplayercontrol::playbackStateChanged,
            this,&detect_video_view::updatePlayPauseButton);
    connect(player_control,&videoplayercontrol::videoOpened,this,
            [this](const QString &,qint64 duration){
        player_stack->setCurrentWidget(video_label);
        play_pause_button->setEnabled(true);
        snapshot_button->setEnabled(true);
        progress_slider->setRange(0,static_cast<int>(qBound<qint64>(
            0,duration,qint64(std::numeric_limits<int>::max()))));
        progress_slider->setValue(0);
        time_label->setText(QString("00:00/%1").arg(formatDuration(duration)));
    });
    connect(player_control,&videoplayercontrol::durationChanged,this,
            [this](qint64 duration){
        progress_slider->setRange(0,static_cast<int>(qBound<qint64>(
            0,duration,qint64(std::numeric_limits<int>::max()))));
    });
    connect(player_control,&videoplayercontrol::positionChanged,this,
            &detect_video_view::updatePlaybackPosition);
    connect(player_control,&videoplayercontrol::videoClosed,this,[this](){
        current_image=QImage();
        video_label->clear();
        player_stack->setCurrentWidget(empty_label);
        play_pause_button->setEnabled(false);
        snapshot_button->setEnabled(false);
        progress_slider->setRange(0,0);
        time_label->setText("00:00/00:00");
    });
    connect(player_control,&videoplayercontrol::errorOccurred,this,
            [this](const QString &message){
        showWarningMessageBox("播放失败",message);
    });
    connect(progress_slider,&QSlider::sliderMoved,this,[this](int position){
        player_control->seek(position);
    });
    connect(progress_slider,&QSlider::sliderReleased,this,[this](){
        player_control->seek(progress_slider->value());
    });
    connect(speed_choose,&QComboBox::currentIndexChanged,this,[this](){
        player_control->setPlaybackRate(speed_choose->currentData().toDouble());
    });
    connect(snapshot_button,&QPushButton::clicked,this,[this](){
        QString savedPath;
        QString errorMessage;
        if(!player_control->saveSnapshot(settinginfo::Image_file_path,
                                         savedPath,errorMessage,
                                         "detect_snapshot")){
            showWarningMessageBox("提示",errorMessage);
            return;
        }
        videocontrol databaseControl;
        if(!databaseControl.createImageRecord(current_camera_id,current_video_id,
                                              savedPath,0,errorMessage)){
            showWarningMessageBox("提示",
                                  QString("截图已保存，但写入image_t失败：\n%1")
                                      .arg(errorMessage));
            return;
        }
        showWarningMessageBox("提示",QString("截屏已保存到:\n%1").arg(savedPath));
    });

    // 目录中的文件被手动删除或新增时，及时重新读取列表。
    video_directory_watcher=new QFileSystemWatcher(this);
    connect(video_directory_watcher,
            &QFileSystemWatcher::directoryChanged,
            this,
            [this](const QString &){
                refreshVideoList();
                updateVideoDirectoryWatcher();
            });
}

void detect_video_view::updateVideoList()
{
    updateVideoDirectoryWatcher();
    videocontrol control;
    QList<video> records;
    const int channelId=channel_choose->currentData().toInt();
    int total=0;
    // 移动侦测列表以 Video_file_path 中实际存在的 detect 视频为准，删除磁盘文件不删除 detect_t。
    control.queryDetectVideoBypage(channelId,
                                   time_choose->date(),
                                   page_size,
                                   (current_page-1)*page_size,
                                   settinginfo::Video_file_path,
                                   records,
                                   total);
    total_page=qMax(1,(total+page_size-1)/page_size);
    if(current_page>total_page){
        current_page=total_page;
        changing_page=true;
        updateVideoList();
        return;
    }
    video_list->clear();
    for(const video &record:records){
        QListWidgetItem *item=new QListWidgetItem(record.getVideo_name());
        item->setData(Qt::UserRole,record.getVideo_path());
        item->setData(Qt::UserRole+1,record.getVideo_id());
        item->setData(Qt::UserRole+2,record.getChannel_id());
        item->setData(Qt::UserRole+3,record.getCamera_id());
        video_list->addItem(item);
    }
    page_label->setText(QString("%1/%2").arg(current_page).arg(total_page));
    changing_page=false;
}

void detect_video_view::updateVideoDirectoryWatcher()
{
    if(!video_directory_watcher){
        return;
    }

    const QString configuredPath=settinginfo::Video_file_path.trimmed();
    const QString directory=configuredPath.isEmpty()
                                 ?QString()
                                 :QDir(configuredPath).absolutePath();
    if(directory==watched_video_directory){
        return;
    }
    if(!watched_video_directory.isEmpty()){
        video_directory_watcher->removePath(watched_video_directory);
    }
    watched_video_directory=directory;
    if(!directory.isEmpty()&&QDir(directory).exists()){
        video_directory_watcher->addPath(directory);
    }
}

void detect_video_view::refreshVideoList()
{
    // 新录像完成或保存目录发生变化时，从第一页重新读取可播放文件。
    current_page=1;
    changing_page=false;
    updateVideoList();
}

void detect_video_view::stopPlayback()
{
    if(player_control){
        player_control->closeVideo();
    }
}

void detect_video_view::playVideo(const QString &videoPath)
{
    player_control->openVideo(videoPath);
}

void detect_video_view::updatePlayPauseButton(bool playing)
{
    play_pause_button->setText(playing?"暂停":"播放");
}

void detect_video_view::updatePlaybackPosition(qint64 position,qint64 duration)
{
    if(!progress_slider->isSliderDown()){
        progress_slider->setValue(static_cast<int>(qBound<qint64>(
            qint64(0),
            position,
            qint64(progress_slider->maximum()))));
    }
    time_label->setText(QString("%1/%2").arg(formatDuration(position)).arg(formatDuration(duration)));
}

void detect_video_view::showCurrentFrame(const QImage &image)
{
    if(image.isNull()){
        return;
    }
    video_label->setPixmap(QPixmap::fromImage(image).scaled(video_label->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void detect_video_view::seekSliderByMouse(QMouseEvent *event)
{
    if(progress_slider->maximum()<=progress_slider->minimum()){
        return;
    }
    const double ratio=qBound(0.0,event->position().x()/static_cast<double>(progress_slider->width()),1.0);
    const int value=progress_slider->minimum()+static_cast<int>(
        (progress_slider->maximum()-progress_slider->minimum())*ratio);
    progress_slider->setValue(value);
    player_control->seek(value);
}

QString detect_video_view::formatDuration(qint64 milliseconds) const
{
    const qint64 seconds=milliseconds/1000;
    const qint64 hours=seconds/3600;
    const qint64 minutes=(seconds/60)%60;
    const qint64 remainSeconds=seconds%60;
    if(hours>0){
        return QString("%1:%2:%3").arg(hours,2,10,QChar('0')).arg(minutes,2,10,QChar('0')).arg(remainSeconds,2,10,QChar('0'));
    }
    return QString("%1:%2").arg(minutes,2,10,QChar('0')).arg(remainSeconds,2,10,QChar('0'));
}

void detect_video_view::showWarningMessageBox(const QString &title,const QString &text)
{
    QMessageBox::information(this,title,text);
}

bool detect_video_view::eventFilter(QObject *watched,QEvent *event)
{
    if(watched==progress_slider&&event->type()==QEvent::MouseButtonPress){
        QMouseEvent *mouseEvent=static_cast<QMouseEvent *>(event);
        if(mouseEvent->button()==Qt::LeftButton){
            seekSliderByMouse(mouseEvent);
            return false;
        }
    }
    return QWidget::eventFilter(watched,event);
}

void detect_video_view::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    showCurrentFrame(current_image);
}

void detect_video_view::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if(!event->spontaneous()&&!changing_page){
        updateVideoList();
    }
}
