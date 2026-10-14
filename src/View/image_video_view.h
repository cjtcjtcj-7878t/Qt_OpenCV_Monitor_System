#ifndef IMAGE_VIDEO_VIEW_H
#define IMAGE_VIDEO_VIEW_H
#include <QImage>
#include <QList>
#include <QStringList>
#include <QWidget>
#include <array>
#include "../Model/imageinfo.h"
class QCalendarWidget; class QComboBox; class QDateEdit; class QEvent;
class QFileSystemWatcher; class QLabel; class QListWidget; class QMouseEvent;
class QPushButton; class QResizeEvent; class QShowEvent; class QSlider;
class QStackedLayout; class QToolButton; class QCheckBox; class QTimer;
class videoplayercontrol; class ImageCanvas;

class image_video_view : public QWidget
{
    Q_OBJECT
public:
    explicit image_video_view(QWidget *parent=nullptr);
public slots:
    void refreshImageList();
    void stopPlayback();
protected:
    bool eventFilter(QObject *,QEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;
private:
    void initQss(); //qss文件优化
    void initLeftWidget();      //初始化左侧时间选择、通道选择、类型选择
    void initRightWidget();     //初始化右侧图片预览
    void initConnections();     //信号与槽函数
    void updateImageList();
    void updateDirectoryWatcher();
    void updateThumbnailSizes();
    void showImage(int index);
    void moveImage(int delta);
    void showGrid();
    void showRelatedVideo();
    void exportImages();
    void showHover();
    void hideHover();
    void requestVideo(const QString &videoPath);
    void resetPlayerUi(const QString &message="暂无播放");
    void showCurrentFrame(const QImage &image);
    void updatePlaybackPosition(qint64 position,qint64 duration);
    void seekSliderByMouse(QMouseEvent *event);
    QString formatDuration(qint64 milliseconds) const;
    void showMessage(const QString &title,const QString &message);

    QWidget *left_widget=nullptr,*right_widget=nullptr;
    QDateEdit *date_choose=nullptr;
    QComboBox *channel_choose=nullptr,*event_choose=nullptr;
    QListWidget *hour_list=nullptr;
    QPushButton *previous_button=nullptr,*next_button=nullptr;
    QLabel *page_label=nullptr,*status_label=nullptr;
    int current_page=1,total_page=1,page_size=9,selected_index=-1;
    bool refresh_pending=false;
    QList<imageinfo> records;
    std::array<QToolButton*,9> thumbs{};
    std::array<QCheckBox*,9> checks{};
    QCheckBox *select_all=nullptr;
    std::array<QImage,9> thumbnail_images;
    QStackedLayout *right_stack=nullptr;
    QWidget *grid_page=nullptr,*image_page=nullptr,*video_page=nullptr;
    ImageCanvas *canvas=nullptr;
    QLabel *image_caption=nullptr,*zoom_label=nullptr;
    QPushButton *related_button=nullptr,*image_previous=nullptr,*image_next=nullptr;
    QLabel *hover_popup=nullptr;
    QTimer *hover_timer=nullptr,*refresh_timer=nullptr;
    int hover_index=-1;
    QString requested_video_path;
    qint64 pending_seek=-1;

    videoplayercontrol *player_control=nullptr;
    QLabel *video_label=nullptr,*empty_label=nullptr;
    QStackedLayout *player_stack=nullptr;
    QPushButton *play_pause_button=nullptr;
    QSlider *progress_slider=nullptr;
    QLabel *time_label=nullptr;
    QComboBox *speed_choose=nullptr;
    QImage current_image;
    QFileSystemWatcher *directory_watcher=nullptr;
    QStringList watched_directories;
};
#endif
