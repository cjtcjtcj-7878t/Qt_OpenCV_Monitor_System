#include "normal_video_view.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>

normal_video_view::normal_video_view(QWidget *parent)
    : QWidget{parent}
{
    //主布局--水平布局
    this->current_page=1;   //分页起始页数
    this->page_size=17; //每页17条数据
    this->total_page=1;
    QHBoxLayout *main_layout=new QHBoxLayout;
    this->setLayout(main_layout);

    left_widget=new QWidget;
    main_layout->addWidget(left_widget,2);

    right_widget=new QWidget;
    main_layout->addWidget(right_widget,6);

    this->init_qss();
    this->init_left_widget();
    this->init_right_widget();
    this->init_connect();
    this->updateVideoList();

}

void normal_video_view::init_qss()
{
    //调用qss文件优化
    QFile file(":/qss/normal_video_replay");
    if(file.open(QFile::ReadOnly)){
        this->setStyleSheet(file.readAll());
        file.close();
    }

}

void normal_video_view::init_left_widget()
{
    QVBoxLayout *left_layout=new QVBoxLayout;
    left_widget->setLayout(left_layout);    //左组件主布局--垂直布局

    //日期选择和通道号选择布局--水平布局
    QHBoxLayout *select_layout=new QHBoxLayout;


    //日期选择
    time_choose=new QDateEdit;
    time_choose->setDisplayFormat("yyyy年MM月dd日");   //显示格式
    time_choose->setDate(QDate::currentDate()); //默认显示当前日期
    time_choose->setCalendarPopup(true);
    time_choose->setMinimumDate(QDate(1900,1,1));       //设置最小选择日期
    time_choose->setMaximumDate(QDate(2100,12,31));    //设置最大选择日期

    time_choose->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    //中文显示
    QLocale local(QLocale::Chinese);
    time_choose->setLocale(local);
    //获取日期选择下拉展开的日期控件
    cal=new QCalendarWidget;
    cal=time_choose->calendarWidget();
    cal->setGridVisible(true);  //显示网格线
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    //显示日历的导航栏(上一个/下一个/选择)
    cal->setNavigationBarVisible(true);
    select_layout->addWidget(time_choose,6);

    //通道号选择
    channel_choose=new QComboBox;
    channel_choose->addItem("所有通道",0);
    channel_choose->addItem("通道1",1);
    channel_choose->addItem("通道2",2);
    channel_choose->addItem("通道3",3);
    channel_choose->addItem("通道4",4);
    select_layout->addWidget(channel_choose,4);

    //下方视频列表
    QVBoxLayout *video_layout=new QVBoxLayout;
    video_list=new QListWidget;

    //模拟显示，不作为最终结果
    // for(int i=0;i<17;i++){
    //     QListWidgetItem *item=new QListWidgetItem(QString("channel1_20111010")+QString::number(i)+"mp4");
    //     video_list->addItem(item);
    //     item->setTextAlignment(Qt::AlignCenter);
    // }

    video_layout->addWidget(video_list,8);

    //下方翻页组件
    QWidget *page_widget=new QWidget;
    QHBoxLayout *page_layout=new QHBoxLayout;
    page_widget->setLayout(page_layout);    //水平布局

    btn_last=new QPushButton("<<");
    btn_next=new QPushButton(">>");

    page_label=new QLabel(QString("%1/%2").arg(current_page).arg(total_page));
    page_label->setAlignment(Qt::AlignCenter);
    page_layout->addWidget(btn_last);
    page_layout->addWidget(page_label);
    page_layout->addWidget(btn_next);
    video_layout->addWidget(page_widget,1);

    left_layout->addLayout(select_layout,1);
    left_layout->addLayout(video_layout,8);

}

void normal_video_view::init_right_widget()
{
    //右组件主布局-垂直布局
    QVBoxLayout *right_layout=new QVBoxLayout;
    right_widget->setLayout(right_layout);

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

    //下方按钮布局--水平布局
    QHBoxLayout *control_layout=new QHBoxLayout;

    //播放按钮
    play_pause_button=new QPushButton("播放");
    play_pause_button->setEnabled(false);
    control_layout->addWidget(play_pause_button);

    //视频进度条
    progress_slider=new QSlider(Qt::Horizontal);
    progress_slider->setRange(0,0);
    progress_slider->installEventFilter(this);
    control_layout->addWidget(progress_slider,8);

    //时间变化
    time_label=new QLabel("00:00/00:00");
    time_label->setMinimumWidth(100);
    time_label->setAlignment(Qt::AlignCenter);
    control_layout->addWidget(time_label);

    //倍速选择
    speed_choose=new QComboBox;
    speed_choose->addItem("1倍速",1.0);
    speed_choose->addItem("1.5倍速",1.5);
    speed_choose->addItem("2倍速",2.0);
    speed_choose->addItem("4倍速",4.0);
    control_layout->addWidget(speed_choose);

    //手动截屏按钮
    snapshot_button=new QPushButton("手动截屏");
    snapshot_button->setEnabled(false);
    control_layout->addWidget(snapshot_button);

    right_layout->addLayout(control_layout,1);

}

void normal_video_view::init_connect()
{
    //向上翻页
    connect(btn_last,&QPushButton::clicked,this,[this](){
        if(this->current_page>1){
            //当前页数大于1允许向上翻页
            this->current_page--;
            this->ispage=true;
            this->updateVideoList();
        }else{
            this->showWarningMessageBox("提示","已是第一页!");
        }
    });

    //向下翻页
    connect(btn_next,&QPushButton::clicked,this,[this](){
        if(this->current_page<this->total_page){
            //当前页数小于总页数允许向下翻页
            this->current_page++;
            this->ispage=true;
            this->updateVideoList();
        }else{
            this->showWarningMessageBox("提示","已是最后一页!");
        }

    });

    //改变时间选择
    connect(time_choose,&QDateEdit::dateChanged,this,[this](){

        this->updateVideoList();
    });

    //改变通道选择
    connect(channel_choose,&QComboBox::currentIndexChanged,this,[this](){

        this->updateVideoList();
    });

    //改变播放视频的选择
    connect(video_list,&QListWidget::itemDoubleClicked,this,[this](const QListWidgetItem *item){
        current_video_id=item->data(Qt::UserRole+1).toInt();
        current_camera_id=item->data(Qt::UserRole+3).toInt();
        this->playVideo(item->data(Qt::UserRole).toString());
    });

    connect(play_pause_button,
            &QPushButton::clicked,
            player_control,
            &videoplayercontrol::togglePlayback);

    connect(player_control,
            &videoplayercontrol::frameReady,
            this,
            [this](const QImage &image){
                current_image=image;
                player_stack->setCurrentWidget(video_label);
                showCurrentFrame(current_image);
            });

    connect(player_control,
            &videoplayercontrol::playbackStateChanged,
            this,
            &normal_video_view::updatePlayPauseButton);

    connect(player_control,
            &videoplayercontrol::videoOpened,
            this,
            [this](const QString &,qint64 duration){
                empty_label->setText("暂无播放");
                player_stack->setCurrentWidget(video_label);
                play_pause_button->setEnabled(true);
                snapshot_button->setEnabled(true);
                progress_slider->setRange(
                    0,
                    static_cast<int>(qBound<qint64>(
                        qint64(0),
                        duration,
                        qint64(std::numeric_limits<int>::max()))));
                progress_slider->setValue(0);
                time_label->setText(QString("00:00/%1").arg(formatDuration(duration)));
            });

    connect(player_control,
            &videoplayercontrol::durationChanged,
            this,
            [this](qint64 duration){
                progress_slider->setRange(
                    0,
                    static_cast<int>(qBound<qint64>(
                        qint64(0),
                        duration,
                        qint64(std::numeric_limits<int>::max()))));
            });

    connect(player_control,
            &videoplayercontrol::positionChanged,
            this,
            &normal_video_view::updatePlaybackPosition);

    connect(player_control,
            &videoplayercontrol::videoClosed,
            this,
            [this](){
                current_image=QImage();
                video_label->clear();
                player_stack->setCurrentWidget(empty_label);
                play_pause_button->setEnabled(false);
                snapshot_button->setEnabled(false);
                progress_slider->setRange(0,0);
                progress_slider->setValue(0);
                time_label->setText("00:00/00:00");
            });

    connect(player_control,
            &videoplayercontrol::errorOccurred,
            this,
            [this](const QString &message){
                resetPlayerUi();
                showWarningMessageBox("播放失败",message);
            });

    connect(progress_slider,&QSlider::sliderMoved,this,[this](int position){
        //拖动进度条
        player_control->seek(position);
    });

    connect(progress_slider,&QSlider::sliderReleased,this,[this](){
        //释放进度条
        player_control->seek(progress_slider->value());
    });

    connect(speed_choose,&QComboBox::currentIndexChanged,this,[this](){
        //选择倍速
        player_control->setPlaybackRate(speed_choose->currentData().toDouble());
    });

    connect(snapshot_button,&QPushButton::clicked,this,[this](){
        //手动截屏
        QString savedPath;
        QString errorMessage;
        if(!player_control->saveSnapshot(settinginfo::Image_file_path,
                                          savedPath,
                                          errorMessage)){
            showWarningMessageBox("提示",errorMessage);
            return;
        }

        videocontrol databaseControl;
        if(!databaseControl.createImageRecord(current_camera_id,
                                              current_video_id,
                                              savedPath,
                                              0,
                                              errorMessage)){
            showWarningMessageBox("提示",QString("截图已保存，但写入image_t失败：\n%1").arg(errorMessage));
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

void normal_video_view::showWarningMessageBox(const QString &title,const QString &text)
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


void normal_video_view::updateVideoList()
{
    updateVideoDirectoryWatcher();
    if(!this->ispage){
        //未进行翻页操作
        this->current_page=1;
    }
    videocontrol vc;
    QList<video> video_list;
    QDate querydate=time_choose->date();    //日历框选择的日期
    int channelid=channel_choose->currentData().toInt();        //选择的通道号
    int total=0;
    // 以 Video_file_path 中实际存在的文件为准，数据库记录只用于补充编号和通道信息。
    vc.queryVideoBypage(channelid,
                        querydate,
                        this->page_size,
                        (this->current_page-1)*this->page_size,
                        settinginfo::Video_file_path,
                        video_list,
                        total);

    //更新到界面
    this->video_list->clear();
    for(auto &v:video_list){
        QListWidgetItem *item=new QListWidgetItem(v.getVideo_name());
        item->setData(Qt::UserRole,v.getVideo_path());
        item->setData(Qt::UserRole+1,v.getVideo_id());
        item->setData(Qt::UserRole+2,v.getChannel_id());
        item->setData(Qt::UserRole+3,v.getCamera_id());
        this->video_list->addItem(item);
    }

    this->total_page=total/this->page_size;     //总页数
    this->total_page=total%this->page_size==0?total_page:total_page+1;
    if(this->total_page<1){
        this->total_page=1;
    }
    if(this->current_page>this->total_page){
        // 文件被手动删除后，当前页可能超出新的最后一页，需要自动回到最后一页。
        this->current_page=this->total_page;
        this->ispage=true;
        this->updateVideoList();
        return;
    }
    //qDebug()<<"总视频数:"<<total;
    page_label->setText(QString("%1/%2").arg(this->current_page).arg(this->total_page));
    this->ispage=false;
    //未翻页
}

void normal_video_view::updateVideoDirectoryWatcher()
{
    if(!video_directory_watcher){
        return;
    }

    const QString configuredPath=settinginfo::Video_file_path.trimmed();
    const QString directory=configuredPath.isEmpty()?QString():QDir(configuredPath).absolutePath();
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

void normal_video_view::refreshVideoList()
{
    // 新录像完成或保存目录发生变化时，从第一页重新读取可播放文件。
    current_page=1;
    ispage=false;
    updateVideoList();
}

void normal_video_view::stopPlayback()
{
    if(player_control){
        player_control->closeVideo();
    }
}

void normal_video_view::playVideo(const QString &videoPath)
{
    const QFileInfo fileInfo(videoPath.trimmed());
    if(!fileInfo.exists()||!fileInfo.isFile()){
        resetPlayerUi();
        showWarningMessageBox("播放失败",QString("视频文件不存在：\n%1").arg(videoPath.trimmed()));
        return;
    }

    // 双击新文件时先清理上一段画面，避免后台切换期间用户看到旧视频。
    current_image=QImage();
    video_label->clear();
    empty_label->setText("正在打开视频...");
    player_stack->setCurrentWidget(empty_label);
    play_pause_button->setEnabled(false);
    snapshot_button->setEnabled(false);
    progress_slider->setRange(0,0);
    time_label->setText("00:00/00:00");
    player_control->openVideo(videoPath);
}

void normal_video_view::resetPlayerUi(const QString &message)
{
    current_image=QImage();
    video_label->clear();
    empty_label->setText(message);
    player_stack->setCurrentWidget(empty_label);
    play_pause_button->setEnabled(false);
    snapshot_button->setEnabled(false);
    progress_slider->setRange(0,0);
    progress_slider->setValue(0);
    time_label->setText("00:00/00:00");
}

void normal_video_view::updatePlayPauseButton(bool playing)
{
    if(!play_pause_button){
        return;
    }
    play_pause_button->setText(playing?"暂停":"播放");
    //根据视频播放状态更改按钮提示：在播放显示“暂停”；暂停显示“播放”
}

void normal_video_view::updatePlaybackPosition(qint64 position,qint64 duration)
{
    //拖动进度条
    if(!progress_slider){
        return;
    }

    if(!progress_slider->isSliderDown()){
        progress_slider->setValue(static_cast<int>(qBound<qint64>(
            qint64(0),
            position,
            qint64(progress_slider->maximum()))));
    }
    time_label->setText(QString("%1/%2").arg(formatDuration(position)).arg(formatDuration(duration)));
}

void normal_video_view::showCurrentFrame(const QImage &image)
{
    //显示QIMAGE画面
    if(image.isNull()||!video_label){
        return;
    }

    video_label->setPixmap(QPixmap::fromImage(image).scaled(video_label->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void normal_video_view::seekSliderByMouse(QMouseEvent *event)
{
    if(!progress_slider||progress_slider->maximum()<=progress_slider->minimum()){
        return;
    }

    const double ratio=qBound(0.0,event->position().x()/static_cast<double>(progress_slider->width()),1.0);
    const int value=progress_slider->minimum()+static_cast<int>((progress_slider->maximum()-progress_slider->minimum())*ratio);
    progress_slider->setValue(value);
    player_control->seek(value);
}

QString normal_video_view::formatDuration(qint64 milliseconds) const
{
    const qint64 seconds=milliseconds/1000;
    const qint64 minutes=seconds/60;
    const qint64 remainSeconds=seconds%60;
    const qint64 hours=minutes/60;
    const qint64 remainMinutes=minutes%60;

    if(hours>0){
        return QString("%1:%2:%3").arg(hours,2,10,QChar('0')).arg(remainMinutes,2,10,QChar('0')).arg(remainSeconds,2,10,QChar('0'));
    }
    return QString("%1:%2").arg(remainMinutes,2,10,QChar('0')).arg(remainSeconds,2,10,QChar('0'));
}

bool normal_video_view::eventFilter(QObject *watched, QEvent *event)
{
    if(watched==progress_slider&&event->type()==QEvent::MouseButtonPress){
        QMouseEvent *mouseEvent=static_cast<QMouseEvent*>(event);
        if(mouseEvent->button()==Qt::LeftButton){
            seekSliderByMouse(mouseEvent);
            return false;
        }
    }

    return QWidget::eventFilter(watched,event);
}

void normal_video_view::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    showCurrentFrame(current_image);
}

void normal_video_view::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if(!event->spontaneous()){
        updateVideoList();
    }
}
