#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <QDateTime>
#include <QString>

class imageinfo
{
public:
    QDateTime relatedVideoStart() const { return related_video_start; }
    void setRelatedVideoStart(const QDateTime &time) { related_video_start = time; }
    bool relatedVideoComplete() const { return related_video_complete; }
    void setRelatedVideoComplete(bool complete) { related_video_complete = complete; }
    int imageId() const;
    void setImageId(int id);

    int cameraId() const;
    void setCameraId(int id);

    int videoId() const;
    void setVideoId(int id);

    QString imageName() const;
    void setImageName(const QString &name);

    qint64 imageSize() const;
    void setImageSize(qint64 size);

    QString imagePath() const;
    void setImagePath(const QString &path);

    QDateTime createTime() const;
    void setCreateTime(const QDateTime &time);

    int eventType() const;
    void setEventType(int type);

    int channelId() const;
    void setChannelId(int id);

    QString relatedVideoPath() const;
    void setRelatedVideoPath(const QString &path);

    QString relatedVideoName() const;
    void setRelatedVideoName(const QString &name);

    QString videoTable() const;
    void setVideoTable(const QString &table);

private:
    QDateTime related_video_start;
    bool related_video_complete = false;
    int image_id = 0;
    int camera_id = 0;
    int video_id = 0;
    QString image_name;
    qint64 image_size = 0;
    QString image_path;
    QDateTime create_time;
    int event_type = 0;
    int channel_id = 0;
    QString related_video_path;
    QString related_video_name;
    QString video_table;
};

#endif // IMAGEINFO_H
