#include "video.h"

video::video() {}

int video::getVideo_id() const
{
    return Video_id;
}

void video::setVideo_id(int newVideo_id)
{
    Video_id = newVideo_id;
}

QString video::getVideo_name() const
{
    return Video_name;
}

void video::setVideo_name(const QString &newVideo_name)
{
    Video_name = newVideo_name;
}

QString video::getVideo_path() const
{
    return Video_path;
}

void video::setVideo_path(const QString &newVideo_path)
{
    Video_path = newVideo_path;
}

int video::getChannel_id() const
{
    return channel_id;
}

void video::setChannel_id(int newChannel_id)
{
    channel_id = newChannel_id;
}

int video::getCamera_id() const
{
    return camera_id;
}

void video::setCamera_id(int newCamera_id)
{
    camera_id = newCamera_id;
}

qint64 video::getFile_size() const
{
    return file_size;
}

void video::setFile_size(qint64 newFile_size)
{
    file_size = newFile_size;
}

QDateTime video::getStart_time() const
{
    return start_time;
}

void video::setStart_time(const QDateTime &newStart_time)
{
    start_time = newStart_time;
}

QDateTime video::getEnd_time() const
{
    return end_time;
}

void video::setEnd_time(const QDateTime &newEnd_time)
{
    end_time = newEnd_time;
}

QDateTime video::getCreate_time() const
{
    return create_time;
}

void video::setCreate_time(const QDateTime &newCreate_time)
{
    create_time = newCreate_time;
}
