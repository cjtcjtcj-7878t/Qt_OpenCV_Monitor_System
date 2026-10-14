#include "camerainfo.h"

camerainfo::camerainfo() {}

int camerainfo::getChannelId() const
{
    return channelId;
}

void camerainfo::setChannelId(int newChannelId)
{
    channelId = newChannelId;
}

int camerainfo::getCameraIndex() const
{
    return cameraIndex;
}

void camerainfo::setCameraIndex(int newCameraIndex)
{
    cameraIndex = newCameraIndex;
}

QString camerainfo::getChannelName() const
{
    return channelName;
}

void camerainfo::setChannelName(const QString &newChannelName)
{
    channelName = newChannelName;
}

QString camerainfo::getCameraName() const
{
    return cameraName;
}

void camerainfo::setCameraName(const QString &newCameraName)
{
    cameraName = newCameraName;
}
