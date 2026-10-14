#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QString>
#include <QtGlobal>
#include <opencv2/videoio.hpp>

class videoplayer
{
public:
    videoplayer();
    ~videoplayer();

    bool open(const QString &videoPath, QString *errorMessage = nullptr);       //打开视频文件
    void close();       //关闭视频文件并释放资源
    bool isOpened() const;

    bool readNextFrame();       //读取视频的下一帧
    bool seekToPosition(qint64 position);   //通过点击进度条跳转到视频的指定位置

    const cv::Mat &currentFrame() const;
    double fps() const;     //获取视频的帧率
    qint64 frameCount() const;
    qint64 currentFrameIndex() const;
    qint64 duration() const;        //获取视频的总时长
    qint64 position() const;

private:
    cv::VideoCapture video_capture;     //打开和读取视频文件的类
    cv::Mat current_frame;      //视频帧：视频由一系列连续的图片高速播放形成，1帧表示1个图片
    double video_fps = 0.0;     //视频的帧率，表示每秒转换的图片，如25帧：1000/25=40表示40毫秒换一个图片展示,倍速越高，这个转换时间越短
    qint64 video_frame_count = 0;   //视频的总帧数:时间(s)x帧率
    qint64 current_frame_index = -1;        //当前的帧数
};

#endif // VIDEOPLAYER_H
