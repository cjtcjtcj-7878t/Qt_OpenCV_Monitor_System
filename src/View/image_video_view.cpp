#include "image_video_view.h"
#include "imagecanvas.h"
#include "../Model/settinginfo.h"
#include "../Service/imagecontrol.h"
#include "../Service/videoplayercontrol.h"
#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <QShortcut>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedLayout>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>
#include <limits>
#include <utility>

namespace {
QImage readImage(const QString &path, const QSize &limit)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize size=reader.size();
    if(size.isValid() && (size.width()>limit.width() || size.height()>limit.height()))
        reader.setScaledSize(size.scaled(limit,Qt::KeepAspectRatio));
    return reader.read();
}
}

image_video_view::image_video_view(QWidget *parent) : QWidget(parent)
{
    auto *layout=new QHBoxLayout(this);
    left_widget=new QWidget(this); right_widget=new QWidget(this);
    left_widget->setMinimumWidth(200); left_widget->setMaximumWidth(280);
    layout->addWidget(left_widget,1); layout->addWidget(right_widget,5);
    initQss(); initLeftWidget(); initRightWidget(); initConnections();
}

void image_video_view::initQss()
{
    setStyleSheet("QPushButton,QToolButton { padding:6px; } "
                  "QToolButton { border:1px solid #cbd5e1; border-radius:6px; background:#f8fafc; } "
                  "QToolButton:hover { border:1px solid #2563eb; background:#eff6ff; }");
}

void image_video_view::initLeftWidget()
{
    auto *layout=new QVBoxLayout(left_widget);
    layout->addWidget(new QLabel("图片筛选",left_widget));
    date_choose=new QDateEdit(QDate::currentDate(),left_widget);
    date_choose->setDisplayFormat("yyyy-MM-dd"); date_choose->setCalendarPopup(true);
    layout->addWidget(date_choose);
    channel_choose=new QComboBox(left_widget);
    channel_choose->addItem("所有通道",0);
    for(int channel=1;channel<=4;++channel) {
        QString name;
        for(const auto &camera:std::as_const(settinginfo::cameras))
            if(camera.getChannelId()==channel) name=camera.getChannelName();
        channel_choose->addItem(QString("通道%1 %2").arg(channel).arg(name),channel);
    }
    layout->addWidget(channel_choose);
    event_choose=new QComboBox(left_widget);
    event_choose->addItem("全部图片",-1);
    event_choose->addItem("自动截图",1);
    event_choose->addItem("手动截图",0);
    layout->addWidget(event_choose);
    hour_list=new QListWidget(left_widget);
    auto *all=new QListWidgetItem("全天",hour_list);
    all->setData(Qt::UserRole,-1);
    for(int h=0;h<24;++h) {
        auto *item=new QListWidgetItem(QString("%1:00 - %2:00").arg(h,2,10,QChar('0')).arg(h+1,2,10,QChar('0')),hour_list);
        item->setData(Qt::UserRole,h);
    }
    hour_list->setCurrentRow(0); layout->addWidget(hour_list,1);
}

void image_video_view::initRightWidget()
{
    auto *layout=new QVBoxLayout(right_widget);
    status_label=new QLabel("按日期和通道浏览图片",right_widget);
    status_label->setWordWrap(true); layout->addWidget(status_label);
    right_stack=new QStackedLayout;
    layout->addLayout(right_stack,1);
    grid_page=new QWidget(right_widget);
    auto *gridLayout=new QVBoxLayout(grid_page);
    auto *grid=new QGridLayout; grid->setSpacing(8);
    for(int i=0;i<9;++i) {
        auto *cell=new QWidget(grid_page);
        auto *cellLayout=new QVBoxLayout(cell); cellLayout->setContentsMargins(0,0,0,0);
        thumbs[i]=new QToolButton(cell);
        thumbs[i]->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        thumbs[i]->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
        thumbs[i]->setMinimumSize(80,70);
        thumbs[i]->setProperty("imageSlot",i); thumbs[i]->installEventFilter(this);
        thumbs[i]->setMouseTracking(true);
        checks[i]=new QCheckBox("选择",cell);
        cellLayout->addWidget(thumbs[i],1); cellLayout->addWidget(checks[i]);
        grid->addWidget(cell,i/3,i%3);
        grid->setRowStretch(i/3,1); grid->setColumnStretch(i%3,1);
        connect(thumbs[i],&QToolButton::clicked,this,[this,i](){showImage(i);});
    }
    gridLayout->addLayout(grid,1);
    auto *pager=new QHBoxLayout;
    previous_button=new QPushButton("上一页",grid_page);
    next_button=new QPushButton("下一页",grid_page);
    page_label=new QLabel("1 / 1",grid_page);
    auto *selectAll=new QCheckBox("选择本页",grid_page);
    select_all=selectAll;
    auto *exportButton=new QPushButton("导出选中图片",grid_page);
    pager->addWidget(selectAll); pager->addWidget(exportButton); pager->addStretch();
    pager->addWidget(previous_button); pager->addWidget(page_label); pager->addWidget(next_button);
    connect(selectAll,&QCheckBox::toggled,this,[this](bool selected){
        for(int i=0;i<records.size();++i) checks[i]->setChecked(selected);
    });
    // 翻页时清除本页全选开关，避免复选框状态误导。
    connect(previous_button,&QPushButton::clicked,selectAll,[selectAll](){selectAll->setChecked(false);});
    connect(next_button,&QPushButton::clicked,selectAll,[selectAll](){selectAll->setChecked(false);});
    connect(exportButton,&QPushButton::clicked,this,&image_video_view::exportImages);
    gridLayout->addLayout(pager); right_stack->addWidget(grid_page);

    image_page=new QWidget(right_widget);
    auto *large=new QVBoxLayout(image_page);
    auto *tools=new QHBoxLayout;
    auto *back=new QPushButton("返回缩略图",image_page);
    auto *reset=new QPushButton("适应窗口",image_page);
    zoom_label=new QLabel("100%",image_page);
    tools->addWidget(back); tools->addWidget(reset); tools->addWidget(zoom_label); tools->addStretch();
    large->addLayout(tools);
    canvas=new ImageCanvas(image_page); large->addWidget(canvas,1);
    canvas->zoomChanged=[this](double zoom){zoom_label->setText(QString("%1%").arg(qRound(zoom*100)));};
    image_caption=new QLabel(image_page); image_caption->setWordWrap(true); large->addWidget(image_caption);
    auto *largeControls=new QHBoxLayout;
    image_previous=new QPushButton("← 上一张",image_page);
    image_next=new QPushButton("下一张 →",image_page);
    related_button=new QPushButton("查看对应视频",image_page);
    largeControls->addWidget(image_previous);largeControls->addWidget(related_button);largeControls->addWidget(image_next);
    large->addLayout(largeControls); right_stack->addWidget(image_page);
    connect(back,&QPushButton::clicked,this,&image_video_view::showGrid);
    connect(reset,&QPushButton::clicked,canvas,&ImageCanvas::resetZoom);
    connect(image_previous,&QPushButton::clicked,this,[this](){moveImage(-1);});
    connect(image_next,&QPushButton::clicked,this,[this](){moveImage(1);});
    connect(related_button,&QPushButton::clicked,this,&image_video_view::showRelatedVideo);
    for(int delta:{-1,1}) {
        auto *shortcut=new QShortcut(QKeySequence(delta<0?Qt::Key_Left:Qt::Key_Right),this);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut,&QShortcut::activated,this,[this,delta](){
            if(right_stack->currentWidget()==image_page) moveImage(delta);
        });
    }

    video_page=new QWidget(right_widget);
    auto *rightLayout=new QVBoxLayout(video_page);
    auto *backToImage=new QPushButton("返回图片",video_page);
    rightLayout->addWidget(backToImage);
    connect(backToImage,&QPushButton::clicked,this,[this](){
        stopPlayback(); if(selected_index>=0) showImage(selected_index); else showGrid();
    });
    player_control=new videoplayercontrol(this);

    video_label=new QLabel(video_page);
    video_label->setObjectName("videoLabel");
    video_label->setAlignment(Qt::AlignCenter);
    video_label->setStyleSheet("background:#111;color:#ddd;");
    empty_label=new QLabel("暂无播放",video_page);
    empty_label->setObjectName("emptyLabel");
    empty_label->setAlignment(Qt::AlignCenter);
    empty_label->setStyleSheet("background:#111;color:#ddd;font-size:24px;");

    player_stack=new QStackedLayout;
    player_stack->addWidget(empty_label);
    player_stack->addWidget(video_label);
    player_stack->setCurrentWidget(empty_label);
    rightLayout->addLayout(player_stack,1);

    QHBoxLayout *controls=new QHBoxLayout;
    play_pause_button=new QPushButton("播放",video_page);
    play_pause_button->setEnabled(false);
    controls->addWidget(play_pause_button);

    progress_slider=new QSlider(Qt::Horizontal,video_page);
    progress_slider->setRange(0,0);
    progress_slider->installEventFilter(this);
    controls->addWidget(progress_slider,8);

    time_label=new QLabel("00:00/00:00",video_page);
    time_label->setMinimumWidth(100);
    time_label->setAlignment(Qt::AlignCenter);
    controls->addWidget(time_label);

    speed_choose=new QComboBox(video_page);
    speed_choose->addItem("1倍速",1.0);
    speed_choose->addItem("1.5倍速",1.5);
    speed_choose->addItem("2倍速",2.0);
    speed_choose->addItem("4倍速",4.0);
    controls->addWidget(speed_choose);
    rightLayout->addLayout(controls);
    right_stack->addWidget(video_page);
    right_stack->setCurrentWidget(grid_page);
    hover_popup=new QLabel(this,Qt::ToolTip);
    hover_popup->setAttribute(Qt::WA_ShowWithoutActivating);
    hover_popup->setAttribute(Qt::WA_TransparentForMouseEvents);
    hover_popup->setStyleSheet("background:#111827; border:2px solid #64748b; padding:4px; color:white;");
    hover_popup->hide();
    hover_timer=new QTimer(this); hover_timer->setSingleShot(true); hover_timer->setInterval(250);
    connect(hover_timer,&QTimer::timeout,this,&image_video_view::showHover);
}

void image_video_view::initConnections()
{
    auto filterChanged=[this](){stopPlayback(); current_page=1; showGrid(); updateImageList();};
    connect(date_choose,&QDateEdit::dateChanged,this,filterChanged);
    connect(channel_choose,&QComboBox::currentIndexChanged,this,filterChanged);
    connect(event_choose,&QComboBox::currentIndexChanged,this,filterChanged);
    connect(hour_list,&QListWidget::currentRowChanged,this,filterChanged);
    connect(previous_button,&QPushButton::clicked,this,[this](){
        if(current_page>1){--current_page;updateImageList();}
    });
    connect(next_button,&QPushButton::clicked,this,[this](){
        if(current_page<total_page){++current_page;updateImageList();}
    });
    connect(play_pause_button,&QPushButton::clicked,
            player_control,&videoplayercontrol::togglePlayback);
    connect(player_control,&videoplayercontrol::frameReady,this,
            [this](const QImage &image){
                if(right_stack->currentWidget()!=video_page) return;
                current_image=image;
                player_stack->setCurrentWidget(video_label);
                showCurrentFrame(image);
            });
    connect(player_control,&videoplayercontrol::videoOpened,this,
            [this](const QString &path,qint64 duration){
                if (right_stack->currentWidget()!=video_page || path!=requested_video_path) return;
                if (pending_seek>=0) {
                    player_control->seek(qBound<qint64>(0,pending_seek,qMax<qint64>(0,duration-1)));
                    pending_seek=-1;
                }
                play_pause_button->setEnabled(true);
                progress_slider->setRange(
                    0,static_cast<int>(qBound<qint64>(
                        0,duration,qint64(std::numeric_limits<int>::max()))));
                progress_slider->setValue(0);
                time_label->setText(QString("00:00/%1").arg(formatDuration(duration)));
            });
    connect(player_control,&videoplayercontrol::playbackStateChanged,this,
            [this](bool playing){
                play_pause_button->setText(playing?"暂停":"播放");
            });
    connect(player_control,&videoplayercontrol::durationChanged,this,
            [this](qint64 duration){
                progress_slider->setRange(
                    0,static_cast<int>(qBound<qint64>(
                        0,duration,qint64(std::numeric_limits<int>::max()))));
            });
    connect(player_control,&videoplayercontrol::positionChanged,
            this,&image_video_view::updatePlaybackPosition);
    connect(player_control,&videoplayercontrol::videoClosed,this,[this](){
        resetPlayerUi();
    });
    connect(player_control,&videoplayercontrol::errorOccurred,this,
            [this](const QString &message){
                resetPlayerUi();
                showMessage("播放失败",message);
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


    refresh_timer=new QTimer(this); refresh_timer->setSingleShot(true); refresh_timer->setInterval(200);
    connect(refresh_timer,&QTimer::timeout,this,[this](){refreshImageList();});
    directory_watcher=new QFileSystemWatcher(this);
    connect(directory_watcher,&QFileSystemWatcher::directoryChanged,this,[this](const QString &){refresh_timer->start();});
}

void image_video_view::updateImageList()
{
    hideHover(); updateDirectoryWatcher();
    imagecontrol control; int total=0; QString warning;
    const int hour=hour_list->currentItem()?hour_list->currentItem()->data(Qt::UserRole).toInt():-1;
    control.queryImagePage(channel_choose->currentData().toInt(),date_choose->date(),
                           page_size,(current_page-1)*page_size,
                           settinginfo::Image_file_path,settinginfo::Video_file_path,
                           records,total,warning,hour,event_choose->currentData().toInt());
    total_page=qMax(1,(total+page_size-1)/page_size);
    if(current_page>total_page){current_page=total_page;updateImageList();return;}
    selected_index=-1;
    { const QSignalBlocker blocker(select_all); select_all->setChecked(false); }
    for(int i=0;i<9;++i){
        const bool valid=i<records.size();
        thumbs[i]->setEnabled(valid);checks[i]->setEnabled(valid);checks[i]->setChecked(false);
        thumbnail_images[i]=valid?readImage(records[i].imagePath(),QSize(480,320)):QImage();
        thumbs[i]->setIcon(QIcon());
        thumbs[i]->setText(valid?records[i].imageName():"暂无图片");
        if(valid && thumbnail_images[i].isNull()) thumbs[i]->setText("图片无法读取\n"+records[i].imageName());
    }
    page_label->setText(QString("%1 / %2").arg(current_page).arg(total_page));
    previous_button->setEnabled(current_page>1);next_button->setEnabled(current_page<total_page);
    status_label->setText(QString("共 %1 张图片；点击查看大图，悬停快速预览").arg(total)
                          +(warning.isEmpty()?QString():"（数据库信息读取异常，部分关联视频可能不可用）"));
    if(!warning.isEmpty()) qWarning()<<warning;
    refresh_pending=false; updateThumbnailSizes();
}

void image_video_view::updateThumbnailSizes()
{
    for(int i=0;i<9;++i){
        if(!thumbs[i]) continue;
        const QSize size(qMax(40,thumbs[i]->width()-18),qMax(30,thumbs[i]->height()-48));
        thumbs[i]->setIconSize(size);
        thumbs[i]->setIcon(QIcon(QPixmap::fromImage(thumbnail_images[i]).scaled(size,Qt::KeepAspectRatio,Qt::SmoothTransformation)));
    }
}

void image_video_view::showImage(int index)
{
    hideHover();
    if(index<0 || index>=records.size()) return;
    selected_index=index;right_stack->setCurrentWidget(image_page);
    const auto &record=records[index];
    // 解码上限限制超大图片内存；普通摄像头截图保留原始像素。
    const QImage image=readImage(record.imagePath(),QSize(4096,4096));
    canvas->setImage(image);
    image_caption->setText(QString("%1 | 通道%2 | %3%4")
                               .arg(record.imageName()).arg(record.channelId())
                               .arg(record.createTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                               .arg(image.isNull()?" | 图片已丢失或无法解码":""));
    related_button->setEnabled(!record.relatedVideoPath().isEmpty()
                               && record.relatedVideoStart().isValid());
    related_button->setToolTip(record.relatedVideoComplete()?record.relatedVideoPath():"关联视频尚未完成，或缺少可靠的录像时间信息");
    image_previous->setEnabled(index>0 || current_page>1);
    image_next->setEnabled(index+1<records.size() || current_page<total_page);
}

void image_video_view::moveImage(int delta)
{
    const int next=selected_index+delta;
    if(next>=0 && next<records.size()){showImage(next);return;}
    if(delta>0 && current_page<total_page){++current_page;updateImageList();showImage(0);}
    else if(delta<0 && current_page>1){--current_page;updateImageList();showImage(records.size()-1);}
}

void image_video_view::showGrid()
{
    hideHover(); right_stack->setCurrentWidget(grid_page);
    if(refresh_pending) updateImageList();
    QTimer::singleShot(0,this,[this](){updateThumbnailSizes();});
}

void image_video_view::showRelatedVideo()
{
    if(selected_index<0 || selected_index>=records.size()) return;
    imageinfo record=records[selected_index];
    // 大图显示期间不打断缩放/拖动；点击关联视频时重新核对是否已经封口。
    if (!record.relatedVideoComplete()) {
        imagecontrol control; QList<imageinfo> fresh; int total=0; QString warning;
        control.queryImagePage(0, record.createTime().date(),
                               std::numeric_limits<int>::max(),0,settinginfo::Image_file_path,
                               settinginfo::Video_file_path,fresh,total,warning);
        for(const auto &candidate:std::as_const(fresh)) {
            if(candidate.imagePath()==record.imagePath()) {record=candidate;break;}
        }
    }
    if(!record.relatedVideoComplete() || !record.relatedVideoStart().isValid()
        || !QFileInfo(record.relatedVideoPath()).isFile()){
        showMessage("无法回放","关联录像尚未完成、文件已丢失或没有可靠的时间信息。");return;
    }
    pending_seek=qMax<qint64>(0,record.relatedVideoStart().msecsTo(record.createTime()));
    requested_video_path=QFileInfo(record.relatedVideoPath()).absoluteFilePath();
    right_stack->setCurrentWidget(video_page);
    requestVideo(requested_video_path);
}

void image_video_view::exportImages()
{
    QList<imageinfo> selected;
    for(int i=0;i<records.size();++i) if(checks[i]->isChecked()) selected.append(records[i]);
    if(selected.isEmpty()){showMessage("导出","请先勾选本页要导出的图片。");return;}
    const QString destination=QFileDialog::getExistingDirectory(this,"选择导出目录（可选择U盘）");
    if(destination.isEmpty()) return;
    int success=0;QStringList failed;
    for(const auto &record:std::as_const(selected)){
        const QFileInfo source(record.imagePath());
        QString target=QDir(destination).filePath(source.fileName());
        for(int n=1;QFileInfo::exists(target);++n)
            target=QDir(destination).filePath(source.completeBaseName()+QString("_%1.").arg(n)+source.suffix());
        if(QFile::copy(record.imagePath(),target)) ++success;else failed.append(record.imageName());
    }
    showMessage("导出结果",QString("已导出 %1 张，失败 %2 张。").arg(success).arg(failed.size())
                                +(failed.isEmpty()?QString():"\n"+failed.join("\n")));
}

void image_video_view::hideHover()
{
    if(hover_timer) hover_timer->stop();
    if(hover_popup) hover_popup->hide();
    hover_index=-1;
}

void image_video_view::showHover()
{
    if(hover_index<0 || hover_index>=records.size() || !isVisible()
        || right_stack->currentWidget()!=grid_page || !thumbs[hover_index]->underMouse()) return;
    QScreen *screen=QGuiApplication::screenAt(QCursor::pos());
    if(!screen) screen=QGuiApplication::primaryScreen();
    if(!screen) return;
    const QRect area=screen->availableGeometry();
    const QSize limit(qMax(1,qMin(640,area.width()-32)),qMax(1,qMin(420,area.height()-32)));
    QImage image=readImage(records[hover_index].imagePath(),limit);
    hover_popup->clear();
    if(image.isNull()) hover_popup->setText("图片无法读取");
    else hover_popup->setPixmap(QPixmap::fromImage(image).scaled(limit,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    hover_popup->adjustSize();
    QPoint pos=QCursor::pos()+QPoint(18,18);
    if(pos.x()+hover_popup->width()>area.right())pos.setX(QCursor::pos().x()-hover_popup->width()-18);
    if(pos.y()+hover_popup->height()>area.bottom())pos.setY(QCursor::pos().y()-hover_popup->height()-18);
    pos.setX(qMax(area.left(),qMin(pos.x(),area.right()-hover_popup->width()+1)));
    pos.setY(qMax(area.top(),qMin(pos.y(),area.bottom()-hover_popup->height()+1)));
    hover_popup->move(pos);hover_popup->show();
}

void image_video_view::updateDirectoryWatcher()
{
    if(!directory_watcher){
        return;
    }
    QStringList desired;
    for(const QString &configured:{settinginfo::Image_file_path,
                                      settinginfo::Video_file_path}){
        const QString path=QDir(configured.trimmed()).absolutePath();
        if(!configured.trimmed().isEmpty()&&QDir(path).exists()
            &&!desired.contains(path,Qt::CaseInsensitive)){
            desired.append(path);
        }
    }
    if(desired==watched_directories){
        return;
    }
    if(!watched_directories.isEmpty()){
        directory_watcher->removePaths(watched_directories);
    }
    watched_directories=desired;
    if(!watched_directories.isEmpty()){
        directory_watcher->addPaths(watched_directories);
    }
}

void image_video_view::requestVideo(const QString &videoPath)
{
    current_image=QImage();
    video_label->clear();
    empty_label->setText("正在打开视频...");
    player_stack->setCurrentWidget(empty_label);
    play_pause_button->setEnabled(false);
    progress_slider->setRange(0,0);
    time_label->setText("00:00/00:00");
    player_control->openVideo(videoPath);
}

void image_video_view::resetPlayerUi(const QString &message)
{
    current_image=QImage();
    video_label->clear();
    empty_label->setText(message);
    player_stack->setCurrentWidget(empty_label);
    play_pause_button->setEnabled(false);
    play_pause_button->setText("播放");
    progress_slider->setRange(0,0);
    progress_slider->setValue(0);
    time_label->setText("00:00/00:00");
}

void image_video_view::showCurrentFrame(const QImage &image)
{
    if(image.isNull()){
        return;
    }
    video_label->setPixmap(QPixmap::fromImage(image).scaled(
        video_label->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void image_video_view::updatePlaybackPosition(qint64 position,qint64 duration)
{
    if(!progress_slider->isSliderDown()){
        progress_slider->setValue(static_cast<int>(qBound<qint64>(
            0,position,qint64(progress_slider->maximum()))));
    }
    time_label->setText(QString("%1/%2")
                            .arg(formatDuration(position),formatDuration(duration)));
}

void image_video_view::seekSliderByMouse(QMouseEvent *event)
{
    if(progress_slider->maximum()<=progress_slider->minimum()){
        return;
    }
    const double ratio=qBound(0.0,
                                event->position().x()
                                    /static_cast<double>(progress_slider->width()),
                                1.0);
    const int value=progress_slider->minimum()
                      +static_cast<int>((progress_slider->maximum()
                                          -progress_slider->minimum())*ratio);
    progress_slider->setValue(value);
    player_control->seek(value);
}

QString image_video_view::formatDuration(qint64 milliseconds) const
{
    const qint64 seconds=milliseconds/1000;
    const qint64 hours=seconds/3600;
    const qint64 minutes=(seconds/60)%60;
    const qint64 remainSeconds=seconds%60;
    if(hours>0){
        return QString("%1:%2:%3")
        .arg(hours,2,10,QChar('0'))
            .arg(minutes,2,10,QChar('0'))
            .arg(remainSeconds,2,10,QChar('0'));
    }
    return QString("%1:%2")
        .arg(minutes,2,10,QChar('0'))
        .arg(remainSeconds,2,10,QChar('0'));
}

void image_video_view::showMessage(const QString &title,const QString &message)
{
    QMessageBox::information(this,title,message);
}

void image_video_view::refreshImageList()
{
    if(!isVisible() || right_stack->currentWidget()!=grid_page){refresh_pending=true;return;}
    updateImageList();
}
void image_video_view::stopPlayback()
{
    hideHover();pending_seek=-1;requested_video_path.clear();
    if(player_control) player_control->closeVideo();
}
bool image_video_view::eventFilter(QObject *watched,QEvent *event)
{
    if(watched==progress_slider && event->type()==QEvent::MouseButtonPress){
        auto *mouse=static_cast<QMouseEvent*>(event);
        if(mouse->button()==Qt::LeftButton){seekSliderByMouse(mouse);return true;}
    }
    if(watched->property("imageSlot").isValid()){
        if(event->type()==QEvent::Enter){
            hideHover();hover_index=watched->property("imageSlot").toInt();hover_timer->start();
        }else if(event->type()==QEvent::Leave || event->type()==QEvent::Hide
                   || event->type()==QEvent::MouseButtonPress){hideHover();}
    }
    return QWidget::eventFilter(watched,event);
}
void image_video_view::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);hideHover();showCurrentFrame(current_image);
    QTimer::singleShot(0,this,[this](){updateThumbnailSizes();});
}
void image_video_view::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if(!event->spontaneous()){
        const int selectedChannel=channel_choose->currentData().toInt();
        { const QSignalBlocker blocker(channel_choose);
            channel_choose->clear();channel_choose->addItem("所有通道",0);
            for(const auto &camera:std::as_const(settinginfo::cameras))
                channel_choose->addItem(QString("通道%1 %2").arg(camera.getChannelId()).arg(camera.getChannelName()),camera.getChannelId());
            channel_choose->setCurrentIndex(qMax(0,channel_choose->findData(selectedChannel)));
        }
        right_stack->setCurrentWidget(grid_page);updateImageList();
    }
}
