#ifndef SETTINGINFO_H
#define SETTINGINFO_H

#include<QString>
#include<QList>
#include<QDebug>
#include<QSet>
#include<QJsonObject>
#include<QJsonDocument>
#include<QJsonArray>
#include<QFile>
#include<QByteArray>
#include<QJsonParseError>
#include<QCamera>
#include<QCameraDevice>
#include<QMediaDevices>
#include<QCoreApplication>
#include<QDir>
#include<QSaveFile>
#include"../Model/camerainfo.h"
class settinginfo
{
public:
    settinginfo();
    static QString Video_file_path;     //视频的本地存储路径
    static QString Image_file_path;     //图片的本地存储路径
    static QList<camerainfo> cameras;       //摄像头信息列表
    static int Time_interval;       //保存时间间隔
    static bool SaveTofile(QString *errorMessage=nullptr);       //将配置信息以json格式保存到本地
    static bool LoadFromfile();     //读取本地保存的json配置文件
    static void initCamera();      //自动检测本地摄像头
    static QString configPath();        //
};

#endif // SETTINGINFO_H
