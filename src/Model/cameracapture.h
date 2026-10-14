#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <QString>
#include <opencv2/videoio.hpp>

class cameracapture
{
public:
    bool openCamera(int deviceIndex, QString *errorMessage = nullptr);
    void closeCamera();
    bool isOpened() const;
    bool readFrame(cv::Mat &frame);

    double fps() const;
    bool startRecording(const QString &filePath,
                        double frameRate,
                        const cv::Size &frameSize,
                        QString *errorMessage = nullptr);
    bool writeFrame(const cv::Mat &frame);
    void stopRecording();
    bool isRecording() const;

private:
    cv::VideoCapture camera_capture;
    cv::VideoWriter video_writer;       //保存帧
    double camera_fps = 25.0;
};

#endif // CAMERACAPTURE_H
