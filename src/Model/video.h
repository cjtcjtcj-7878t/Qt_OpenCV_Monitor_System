#ifndef VIDEO_H
#define VIDEO_H

#include<QString>
#include<QList>
#include<QDebug>
#include<QFile>
#include<QDateTime>
class video
{
public:
    video();
    int getVideo_id() const;
    void setVideo_id(int newVideo_id);

    QString getVideo_name() const;
    void setVideo_name(const QString &newVideo_name);

    QString getVideo_path() const;
    void setVideo_path(const QString &newVideo_path);

    int getChannel_id() const;
    void setChannel_id(int newChannel_id);

    int getCamera_id() const;
    void setCamera_id(int newCamera_id);

    qint64 getFile_size() const;
    void setFile_size(qint64 newFile_size);

    QDateTime getStart_time() const;
    void setStart_time(const QDateTime &newStart_time);

    QDateTime getEnd_time() const;
    void setEnd_time(const QDateTime &newEnd_time);

    QDateTime getCreate_time() const;
    void setCreate_time(const QDateTime &newCreate_time);

private:
    int Video_id = 0;       //视频编号
    QString Video_name; //视频名称
    QString Video_path; //视频存储路径
    int channel_id = 0;     //拍摄该视频的通道号
    int camera_id = 0;      //camera_t.Id，作为 video_t.Camera_id 外键
    qint64 file_size = 0;
    QDateTime start_time;
    QDateTime end_time;
    QDateTime create_time;      //视频创建时间
};

#endif // VIDEO_H
