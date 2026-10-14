#ifndef CAMERAMODEL_H
#define CAMERAMODEL_H

#include <QString>
#include <QList>

class camerainfo;

class cameramodel
{
public:
    // 根据通道号找到 camera_t.Id 和 OpenCV 设备编号。
    bool findByChannel(int channelId,int &cameraId,int &cameraIndex,QString *errorMessage = nullptr) const;

    bool syncCameras(const QList<camerainfo> &cameras,QString *errorMessage = nullptr) const;
};

#endif // CAMERAMODEL_H
