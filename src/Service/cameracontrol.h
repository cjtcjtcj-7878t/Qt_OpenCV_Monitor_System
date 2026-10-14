#ifndef CAMERACONTROL_H
#define CAMERACONTROL_H

#include <QList>
#include <QString>

#include "../Model/camerainfo.h"
#include "../Model/cameramodel.h"

class cameracontrol
{
public:
    bool syncCameras(const QList<camerainfo> &cameras,QString &errorMessage) const;

private:
    cameramodel camera_model;
};

#endif // CAMERACONTROL_H
