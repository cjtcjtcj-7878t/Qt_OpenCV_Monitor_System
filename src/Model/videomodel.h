#ifndef VIDEOMODEL_H
#define VIDEOMODEL_H

#include"../util/mysqlconnect.h"
#include"video.h"
#include<QList>
#include<QString>
#include<QDate>
#include<QStringList>
#include<QSqlQuery>
class videomodel
{
public:
    videomodel();
    //数据库分页查询普通回看视频

    //查询当前页数据,根据视频创建时间倒序排序显示
    bool queryVideolist(int channelid,
                        QDate date,
                        int limit,
                        int offset,
                        const QString &videoDirectory,
                        QList<video> &videoList);
    bool queryDetectVideolist(int channelid,
                              QDate date,
                              int limit,
                              int offset,
                              const QString &videoDirectory,
                              QList<video> &videoList);
    bool insertVideo(video &v,QString *errorMessage=nullptr); // 插入video_t表中视频
    bool finishVideo(int videoId,qint64 fileSize,const QDateTime &endTime,
                     QString *errorMessage=nullptr);
    bool insertDetectVideo(video &v,QString *errorMessage=nullptr);
    bool finishDetectVideo(int videoId,qint64 fileSize,const QDateTime &endTime,
                           QString *errorMessage=nullptr);

    //查询符合条件的总页数
    int GetVideostotal(int channelid,QDate date,const QString &videoDirectory);
    int GetDetectVideostotal(int channelid,QDate date,const QString &videoDirectory);

private:
    bool queryTable(const QString &tableName,
                    int channelid,
                    QDate date,
                    const QString &videoDirectory,
                    QList<video> &videoList);
    bool queryTablePage(const QString &tableName,
                        int channelid,
                        QDate date,
                        int limit,
                        int offset,
                        const QString &videoDirectory,
                        QList<video> &videoList);
};

#endif // VIDEOMODEL_H
