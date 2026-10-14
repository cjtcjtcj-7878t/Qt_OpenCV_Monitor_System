#include "imageinfo.h"

int imageinfo::imageId() const
{
    return image_id;
}

void imageinfo::setImageId(int id)
{
    image_id = id;
}

int imageinfo::cameraId() const
{
    return camera_id;
}

void imageinfo::setCameraId(int id)
{
    camera_id = id;
}

int imageinfo::videoId() const
{
    return video_id;
}

void imageinfo::setVideoId(int id)
{
    video_id = id;
}

QString imageinfo::imageName() const
{
    return image_name;
}

void imageinfo::setImageName(const QString &name)
{
    image_name = name;
}

qint64 imageinfo::imageSize() const
{
    return image_size;
}

void imageinfo::setImageSize(qint64 size)
{
    image_size = size;
}

QString imageinfo::imagePath() const
{
    return image_path;
}

void imageinfo::setImagePath(const QString &path)
{
    image_path = path;
}

QDateTime imageinfo::createTime() const
{
    return create_time;
}

void imageinfo::setCreateTime(const QDateTime &time)
{
    create_time = time;
}

int imageinfo::eventType() const
{
    return event_type;
}

void imageinfo::setEventType(int type)
{
    event_type = type;
}

int imageinfo::channelId() const
{
    return channel_id;
}

void imageinfo::setChannelId(int id)
{
    channel_id = id;
}

QString imageinfo::relatedVideoPath() const
{
    return related_video_path;
}

void imageinfo::setRelatedVideoPath(const QString &path)
{
    related_video_path = path;
}

QString imageinfo::relatedVideoName() const
{
    return related_video_name;
}

void imageinfo::setRelatedVideoName(const QString &name)
{
    related_video_name = name;
}

QString imageinfo::videoTable() const
{
    return video_table;
}

void imageinfo::setVideoTable(const QString &table)
{
    video_table = table;
}
