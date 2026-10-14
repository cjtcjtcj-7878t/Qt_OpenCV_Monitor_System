#ifndef CAMERAINFO_H
#define CAMERAINFO_H


#include<QString>
#include<QCameraDevice>
class camerainfo
{
public:
    camerainfo();
    int getChannelId() const;
    void setChannelId(int newChannelId);

    int getCameraIndex() const;
    void setCameraIndex(int newCameraIndex);

    QString getChannelName() const;
    void setChannelName(const QString &newChannelName);

    QString getCameraName() const;
    void setCameraName(const QString &newCameraName);

private:
    int channelId = 0;      //摄像头的通道号
    int cameraIndex = -1;   //摄像头在 OpenCV 中的设备编号，虚拟摄像头为 -1
    QString channelName;    //通道名称(自定义名字)
    QString cameraName;     //摄像头名称，系统默认读取的名称，不可更改,不存在的摄像头为"virtual camera"
};

#endif // CAMERAINFO_H
