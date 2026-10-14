#include "settinginfo.h"


//声明静态变量的存在
QString settinginfo::Video_file_path=QString();
QString settinginfo::Image_file_path=QString();
int settinginfo::Time_interval=0;
QList<camerainfo> settinginfo::cameras=QList<camerainfo>();



settinginfo::settinginfo() {}



void settinginfo::initCamera()
{
    // //先清空原来的摄像头配置
    // settinginfo::cameras.clear();
    // //自动获取本机可用摄像头设备
    // QList<QCameraDevice> cameraList=QMediaDevices::videoInputs();

    // if(cameraList.empty()){
    //     qDebug()<<"本机器无可用摄像头设备";
    //     return;
    // }
    // qDebug()<<"本机器检测到"<<cameraList.size()<<"个摄像头设备";
    // int i=0;
    // for(i=0;i<cameraList.size()&&settinginfo::cameras.size()<4;i++){
    //     QCameraDevice camera=cameraList.at(i);
    //     QString cameraName=camera.description();
    //     QString cameraId=camera.id();
    //     bool isDefault=camera.isDefault();

    //     //排除红外摄像头
    //     if(cameraName.contains("IR")){
    //         continue;
    //     }

    //     //打印摄像头信息
    //     qDebug()<<"------------- 摄像头信息-------------";
    //     qDebug()<<"摄像头名称:"<<cameraName;
    //     qDebug()<<"摄像头ID:"<<cameraId;
    //     qDebug()<<"通道号:"<<i+1;
    //     qDebug()<<"是否默认摄像头:"<<(isDefault?"是":"否");


    //     //插入settinginfo的QList<camerainfo>
    //     camerainfo cameraInfo;
    //     cameraInfo.setCameraName(cameraName);
    //     cameraInfo.setChannelName(cameraName);
    //     cameraInfo.setChannelId(settinginfo::cameras.size()+1);
    //     settinginfo::cameras.append(cameraInfo);

    // }

    // //摄像头不足4个，则补充virtual camera
    // while(settinginfo::cameras.size() < 4)
    // {
    //     int index = settinginfo::cameras.size();

    //     camerainfo cameraInfo;

    //     cameraInfo.setChannelId(index + 1);
    //     cameraInfo.setCameraName("virtual camera");
    //     cameraInfo.setChannelName("virtual camera");

    //     settinginfo::cameras.append(cameraInfo);
    // }
    // qDebug()<<"--------------------------------------";
    // qDebug()<<"";
    // qDebug() << "最终 cameras 数量:"<< settinginfo::cameras.size();
    // qDebug()<< "其中虚拟摄像头数量:"<<settinginfo::cameras.size()-QMediaDevices::videoInputs().size();
    // qDebug()<< "实际工作摄像头数量:"<<QMediaDevices::videoInputs().size();
    // qDebug()<<"";
    // qDebug()<<"--------------------------------------";
    // for(i--;i<4;i++){
    //     camerainfo cameraInfo;
    //     cameraInfo.setChannelId(i+1);
    //     cameraInfo.setCameraName("virtual camera");
    //     cameraInfo.setChannelName("virtual camera");
    //     settinginfo::cameras.append(cameraInfo);
    // }
    cameras.clear();


    const QList<QCameraDevice> devices =QMediaDevices::videoInputs();


    for (int systemIndex=0;systemIndex<devices.size();++systemIndex) {
        if (cameras.size() >= 4) {
            break;
        }

        const QCameraDevice &device=devices.at(systemIndex);
        const QString name = device.description();


        //去除红外摄像头

        if (name.contains("IR")) {
            continue;
        }

        camerainfo camera;
        camera.setCameraName(name);
        camera.setChannelName(name);
        camera.setChannelId(cameras.size() + 1);
        // 保存设备在系统列表中的原始位置，避免跳过红外设备后编号错位。
        camera.setCameraIndex(systemIndex);
        cameras.append(camera);
    }


    // 包括完全没有摄像头的情况，都补齐到4个

    while (cameras.size() < 4) {

        camerainfo camera;
        camera.setCameraName("virtual camera");
        camera.setChannelName("virtual camera");
        camera.setChannelId(cameras.size() + 1);
        camera.setCameraIndex(-1);
        cameras.append(camera);

}
}


bool settinginfo::SaveTofile(QString *errorMessage)
{

    if (errorMessage) {
        errorMessage->clear();
    };
    const QString configPath =settinginfo::configPath();
    //保存配置信息到本地
    QSaveFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }
    //写入基础配置
    QJsonObject rootObj;
    rootObj["video_file_path"]=settinginfo::Video_file_path;
    rootObj["image_file_path"]=settinginfo::Image_file_path;
    rootObj["time_interval"]=settinginfo::Time_interval;

    //写入摄像头信息(摄像头名称，通道名称，通道号)
    QJsonArray cameraArray;
    for(camerainfo &cam:settinginfo::cameras){
        QJsonObject camObj;
        camObj["cameraName"]=cam.getCameraName();   //赋值摄像头名称
        camObj["channelName"]=cam.getChannelName();     //赋值通道号名称
        camObj["channel"]=cam.getChannelId();       //赋值通道号
        cameraArray.append(camObj);
    }
    //数组形式记录摄像头相关信息
    rootObj["Cameras"]=cameraArray;

    //写入文件
    QJsonDocument doc(rootObj);
    const QByteArray data = doc.toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        if (errorMessage) *errorMessage = file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }
    return true;
}


//从本地文件读取配置信息
bool settinginfo::LoadFromfile()
{
    // QString configPath="./config.json";
    // QFile file(configPath);

    // if(!file.open(QIODevice::ReadOnly|QIODevice::Text)){
    //     settinginfo::initCamera();
    //     return;
    // }

    // //读取文件内容并解析为json文档
    // QByteArray jsonData=file.readAll();
    // QJsonParseError jsonerror;
    // QJsonDocument doc=QJsonDocument::fromJson(jsonData,&jsonerror);

    // //json解析失败
    // if(jsonerror.error!=QJsonParseError::NoError||!doc.isObject()){
    //     qDebug()<<"config.json文件解析失败"<<jsonerror.errorString();

    //     file.close();

    //     settinginfo::cameras.clear();
    //     settinginfo::initCamera();
    //     return;
    // }

    // //解析根对象

    // QJsonObject jsObj=doc.object();

    // //读取文件基础配置信息并赋值到静态变量
    // settinginfo::Video_file_path=jsObj["video_file_path"].toString();
    // settinginfo::Image_file_path=jsObj["image_file_path"].toString();
    // settinginfo::Time_interval=jsObj["time_interval"].toInt();

    // //读取摄像头列表信息并赋值到静态摄像头QList

    // settinginfo::cameras.clear();
    // QJsonArray cameraArray=jsObj["Cameras"].toArray();
    // for(int i=0;i<cameraArray.size()&&i<4;i++){
    //     //摄像头数组每个元素转为用于赋值的对象
    //     QJsonObject camObj=cameraArray[i].toObject();
    //     camerainfo cam;     //用于插入QList
    //     cam.setCameraName(camObj["cameraName"].toString());
    //     cam.setChannelName(camObj["channelName"].toString());
    //     cam.setChannelId(camObj["channel"].toInt());

    //     settinginfo::cameras.append(cam);   //插入QList<camerainfo>
    // }

    // //不足4个摄像头就补齐virtual camera
    // while(settinginfo::cameras.size()<4){
    //     camerainfo cam;
    //     int index=settinginfo::cameras.size();
    //     cam.setCameraName(QString("virtual camera"));
    //     cam.setChannelName(QString("通道%1").arg(index+1));
    //     cam.setChannelId(index+1);

    //     //插入QList
    //     settinginfo::cameras.append(cam);
    // }

    // //关闭文件
    // file.close();

    Video_file_path.clear();
    Image_file_path.clear();
    Time_interval = 30;
    QString configPath=settinginfo::configPath();
    QFile file(configPath);

    if(!file.open(QIODevice::ReadOnly|QIODevice::Text)){
        settinginfo::initCamera();
        return false;
    }

    //读取文件内容并解析为json文档
    QByteArray jsonData=file.readAll();
    QJsonParseError jsonerror;
    QJsonDocument doc=QJsonDocument::fromJson(jsonData,&jsonerror);

    //json解析失败
    if(jsonerror.error!=QJsonParseError::NoError||!doc.isObject()){
        qDebug()<<"config.json文件解析失败"<<jsonerror.errorString();

        file.close();

        settinginfo::cameras.clear();
        settinginfo::initCamera();
        return false;
    }

    //解析根对象

    QJsonObject jsObj=doc.object();
    // 文件存在还不够，必须是完整有效的配置。
    const int interval = jsObj["time_interval"].toInt(-1);  //录制时间间隔
    const QJsonArray entries = jsObj["Cameras"].toArray();  //获取摄像头信息数组
    bool valid = jsObj["video_file_path"].isString()    //视频路径存在且不为空
                 && !jsObj["video_file_path"].toString().trimmed().isEmpty()
                 && jsObj["image_file_path"].isString() //图片路径存在且不为空
                 && !jsObj["image_file_path"].toString().trimmed().isEmpty()
                 && (interval == 15 || interval == 30 || interval == 45 || interval == 60)  //录制间隔存在
                 && entries.size() == 4;    //数组大小为4

    //保存不重复的通道号的集合
    QSet<int> channels;
    for (const QJsonValue &entry : entries) {
        const QJsonObject camera = entry.toObject();
        const int channel = camera["channel"].toInt(-1);
        if (!entry.isObject() || !camera["cameraName"].isString()
            || !camera["channelName"].isString()
            || channel < 1 || channel > 4 || channels.contains(channel)) {
            valid = false;
        }
        channels.insert(channel);
    }
    if (!valid) {
        qWarning() << "配置不完整或无效，请重新设置：" << configPath;
        initCamera();
        return false;
    }


    //读取文件基础配置信息并赋值到静态变量
    settinginfo::Video_file_path=jsObj["video_file_path"].toString();
    settinginfo::Image_file_path=jsObj["image_file_path"].toString();
    settinginfo::Time_interval=jsObj["time_interval"].toInt();

    //读取摄像头列表信息并赋值到静态摄像头QList

    // 先重新检测当前设备，避免 config.json 中的旧设备编号继续使用。
    initCamera();
    const QList<camerainfo> detectedCameras=cameras;
    cameras.clear();
    QJsonArray cameraArray=jsObj["Cameras"].toArray();
    for(int i=0;i<cameraArray.size()&&i<4;i++){
        //摄像头数组每个元素转为用于赋值的对象
        QJsonObject camObj=cameraArray[i].toObject();
        camerainfo cam;
        if(i<detectedCameras.size()){
            cam=detectedCameras.at(i);
        }
        cam.setChannelName(camObj["channelName"].toString());
        cam.setChannelId(camObj["channel"].toInt());

        settinginfo::cameras.append(cam);   //插入QList<camerainfo>
    }

    //不足4个摄像头就补齐virtual camera
    while(settinginfo::cameras.size()<4){
        camerainfo cam;
        int index=settinginfo::cameras.size();
        cam.setCameraName(QString("virtual camera"));
        cam.setChannelName(QString("通道%1").arg(index+1));
        cam.setChannelId(index+1);

        //插入QList
        settinginfo::cameras.append(cam);
    }

    //关闭文件
    file.close();
    return true;


}

QString settinginfo::configPath()
{
    //获取配置文件，config.json配置文件与exe程序在同一bin文件夹下
    return QDir(QCoreApplication::applicationDirPath()).filePath("config.json");
}

