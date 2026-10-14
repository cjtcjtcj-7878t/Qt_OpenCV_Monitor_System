#include "cameracontrol.h"

bool cameracontrol::syncCameras(const QList<camerainfo> &cameras,QString &errorMessage) const
{
    return camera_model.syncCameras(cameras,&errorMessage);
}
