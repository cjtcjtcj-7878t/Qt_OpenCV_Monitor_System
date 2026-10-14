#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include "imageinfo.h"

#include <QDate>
#include <QList>
#include <QString>

class imagemodel
{
public:
    bool insertImage(imageinfo &image,QString *errorMessage=nullptr);

    // 以本地图片目录为准查询回看列表，并从数据库补充关联视频信息。
    bool queryImages(int channelId,
                     const QDate &date,
                     int limit,
                     int offset,
                     const QString &imageDirectory,
                     const QString &videoDirectory,
                     QList<imageinfo> &images,
                     int &total,
                     QString *errorMessage=nullptr,
                     int hour=-1, int eventType=-1) const;
};

#endif // IMAGEMODEL_H
