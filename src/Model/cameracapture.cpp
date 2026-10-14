#include "cameracapture.h"

#include <cmath>

bool cameracapture::openCamera(int deviceIndex, QString *errorMessage)
{
    if(errorMessage){
        errorMessage->clear();
    }

    closeCamera();
    if(deviceIndex<0||!camera_capture.open(deviceIndex,cv::CAP_ANY)){
        if(errorMessage){
            *errorMessage=QString("无法打开摄像头，设备编号：%1").arg(deviceIndex);
        }
        return false;
    }

    camera_fps=camera_capture.get(cv::CAP_PROP_FPS);
    if(!std::isfinite(camera_fps)||camera_fps<=0.0){
        camera_fps=25.0;
    }
    return true;
}

void cameracapture::closeCamera()
{
    stopRecording();
    if(camera_capture.isOpened()){
        camera_capture.release();
    }
    camera_fps=25.0;
}

bool cameracapture::isOpened() const
{
    return camera_capture.isOpened();
}

bool cameracapture::readFrame(cv::Mat &frame)
{
    if(!camera_capture.isOpened()){
        return false;
    }
    return camera_capture.read(frame)&&!frame.empty();
}

double cameracapture::fps() const
{
    return camera_fps;
}

bool cameracapture::startRecording(const QString &filePath,double frameRate,const cv::Size &frameSize,QString *errorMessage)
{
    if(errorMessage){
        errorMessage->clear();
    }

    stopRecording();
    const QByteArray encodedPath=filePath.toUtf8();
    const double validFrameRate=std::isfinite(frameRate)&&frameRate>0.0?frameRate:25.0;

    // MJPG + AVI 在本地 OpenCV 环境中兼容性较好，回放仍由 OpenCV 读取。
    if(!video_writer.open(encodedPath.constData(),
                          cv::VideoWriter::fourcc('M','J','P','G'),
                          validFrameRate,
                          frameSize,
                          true)){
        if(errorMessage){
            *errorMessage=QString("无法创建录像文件：\n%1").arg(filePath);
        }
        return false;
    }
    return true;
}

bool cameracapture::writeFrame(const cv::Mat &frame)
{
    if(!video_writer.isOpened()||frame.empty()){
        return false;
    }
    video_writer.write(frame);
    return true;
}

void cameracapture::stopRecording()
{
    if(video_writer.isOpened()){
        video_writer.release();
    }
}

bool cameracapture::isRecording() const
{
    return video_writer.isOpened();
}
