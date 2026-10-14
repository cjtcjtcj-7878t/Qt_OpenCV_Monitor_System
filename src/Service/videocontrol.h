#ifndef VIDEOCONTROL_H
#define VIDEOCONTROL_H

#include"../Model/imagemodel.h"
#include"../Model/videomodel.h"
#include<QDebug>
class videocontrol
{
public:
    videocontrol();
    void queryVideoBypage(int channelid,
                          QDate date,
                          int limit,
                          int offset,
                          const QString &videoDirectory,
                          QList<video> &videoList,
                          int &total);    //分页查询普通视频
    void queryDetectVideoBypage(int channelid,
                                QDate date,
                                int limit,
                                int offset,
                                const QString &videoDirectory,
                                QList<video> &videoList,
                                int &total);    //分页查询移动侦测视频
    bool createVideoRecord(int cameraId,
                           const QString &videoName,
                           const QString &videoPath,
                           const QDateTime &startTime,
                           int &videoId,
                           QString &errorMessage);
    bool finishVideoRecord(int videoId,
                           qint64 fileSize,
                           const QDateTime &endTime,
                           QString &errorMessage);
    bool createDetectVideoRecord(int cameraId,
                                 const QString &videoName,
                                 const QString &videoPath,
                                 const QDateTime &startTime,
                                 int &videoId,
                                 QString &errorMessage);
    bool finishDetectVideoRecord(int videoId,
                                 qint64 fileSize,
                                 const QDateTime &endTime,
                                 QString &errorMessage);
    bool createImageRecord(int cameraId,
                           int videoId,
                           const QString &imagePath,
                           int eventType,
                           QString &errorMessage);
private:
    videomodel vm;
    imagemodel im;
};

#endif // VIDEOCONTROL_H
