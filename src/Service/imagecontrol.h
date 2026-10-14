#ifndef IMAGECONTROL_H
#define IMAGECONTROL_H

#include "../Model/imagemodel.h"

// 图片回看控制层：View 只提出筛选条件，不直接访问数据库。
class imagecontrol
{
public:
    bool queryImagePage(int channelId,
                        const QDate &date,
                        int limit,
                        int offset,
                        const QString &imageDirectory,
                        const QString &videoDirectory,
                        QList<imageinfo> &images,
                        int &total,
                        QString &errorMessage,
                        int hour=-1, int eventType=-1) const;

private:
    imagemodel model;
};

#endif // IMAGECONTROL_H
