#include "videocontrol.h"

#include <QFileInfo>

videocontrol::videocontrol() {}

void videocontrol::queryVideoBypage(int channelid,
                                    QDate date,
                                    int limit,
                                    int offset,
                                    const QString &videoDirectory,
                                    QList<video> &videoList,
                                    int &total)
{
    vm.queryVideolist(channelid,date,limit,offset,videoDirectory,videoList);
    total=vm.GetVideostotal(channelid,date,videoDirectory);
}

void videocontrol::queryDetectVideoBypage(int channelid,
                                          QDate date,
                                          int limit,
                                          int offset,
                                          const QString &videoDirectory,
                                          QList<video> &videoList,
                                          int &total)
{
    vm.queryDetectVideolist(channelid,date,limit,offset,videoDirectory,videoList);
    total=vm.GetDetectVideostotal(channelid,date,videoDirectory);
}

bool videocontrol::createVideoRecord(int cameraId,
                                     const QString &videoName,
                                     const QString &videoPath,
                                     const QDateTime &startTime,
                                     int &videoId,
                                     QString &errorMessage)
{
    videoId=0;
    errorMessage.clear();

    video record;
    record.setCamera_id(cameraId);
    record.setVideo_name(videoName);
    record.setVideo_path(videoPath);
    record.setFile_size(0);
    record.setStart_time(startTime);
    record.setEnd_time(startTime);
    record.setCreate_time(startTime);

    if(!vm.insertVideo(record,&errorMessage)){
        return false;
    }

    videoId=record.getVideo_id();
    return videoId>0;
}

bool videocontrol::finishVideoRecord(int videoId,
                                     qint64 fileSize,
                                     const QDateTime &endTime,
                                     QString &errorMessage)
{
    return vm.finishVideo(videoId,fileSize,endTime,&errorMessage);
}

bool videocontrol::createDetectVideoRecord(int cameraId,
                                           const QString &videoName,
                                           const QString &videoPath,
                                           const QDateTime &startTime,
                                           int &videoId,
                                           QString &errorMessage)
{
    videoId=0;
    errorMessage.clear();

    // detect_t 与 video_t 字段相同，但必须写入独立的移动侦测表。
    video record;
    record.setCamera_id(cameraId);
    record.setVideo_name(videoName);
    record.setVideo_path(videoPath);
    record.setFile_size(0);
    record.setStart_time(startTime);
    record.setEnd_time(startTime);
    record.setCreate_time(startTime);

    if(!vm.insertDetectVideo(record,&errorMessage)){
        return false;
    }
    videoId=record.getVideo_id();
    return videoId>0;
}

bool videocontrol::finishDetectVideoRecord(int videoId,
                                           qint64 fileSize,
                                           const QDateTime &endTime,
                                           QString &errorMessage)
{
    return vm.finishDetectVideo(videoId,fileSize,endTime,&errorMessage);
}

bool videocontrol::createImageRecord(int cameraId,
                                     int videoId,
                                     const QString &imagePath,
                                     int eventType,
                                     QString &errorMessage)
{
    errorMessage.clear();
    const QFileInfo fileInfo(imagePath);
    if(!fileInfo.exists()||!fileInfo.isFile()){
        errorMessage="截图文件不存在，无法写入数据库";
        return false;
    }
    if(cameraId<=0||videoId<=0){
        errorMessage="截图缺少关联的摄像头或视频编号";
        return false;
    }

    imageinfo record;
    record.setCameraId(cameraId);
    record.setVideoId(videoId);
    record.setImageName(fileInfo.fileName());
    record.setImageSize(fileInfo.size());
    record.setImagePath(fileInfo.absoluteFilePath());
    record.setCreateTime(QDateTime::currentDateTime());
    record.setEventType(eventType);
    return im.insertImage(record,&errorMessage);
}
