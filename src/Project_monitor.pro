QT += widgets sql multimedia network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    Model/cameracapture.cpp \
    Model/camerainfo.cpp \
    Model/cameramodel.cpp \
    Model/imageinfo.cpp \
    Model/imagemodel.cpp \
    Model/settinginfo.cpp \
    Model/usermodel.cpp \
    Model/video.cpp \
    Model/videomodel.cpp \
    Model/videoplayer.cpp \
    Service/cameracontrol.cpp \
    Service/detectworker.cpp \
    Service/imagecontrol.cpp \
    Service/logincontrol.cpp \
    Service/monitorcontrol.cpp \
    Service/monitorworker.cpp \
    Service/videocontrol.cpp \
    Service/videoplayercontrol.cpp \
    Service/videoplayerworker.cpp \
    View/captchalable.cpp \
    View/detect_video_view.cpp \
    View/image_video_view.cpp \
    View/login_view.cpp \
    View/main_view.cpp \
    View/monitor_view.cpp \
    View/normal_video_view.cpp \
    View/setting_view.cpp \
    Yolo/Detect.cpp \
    Yolo/YOLO.cpp \
    main.cpp \
    util/dbconnectionpool.cpp \
    util/mysqlconnect.cpp \
    util/winmanage.cpp \


HEADERS += \
    Model/cameracapture.h \
    Model/camerainfo.h \
    Model/cameramodel.h \
    Model/imagehoverpreview.h \
    Model/imageinfo.h \
    Model/imagemodel.h \
    Model/recordingtimeline.h \
    Model/settinginfo.h \
    Model/usermodel.h \
    Model/video.h \
    Model/videomodel.h \
    Model/videoplayer.h \
    Service/cameracontrol.h \
    Service/detectworker.h \
    Service/imagecontrol.h \
    Service/logincontrol.h \
    Service/monitorcontrol.h \
    Service/monitorworker.h \
    Service/videocontrol.h \
    Service/videoplayercontrol.h \
    Service/videoplayerworker.h \
    View/captchalable.h \
    View/detect_video_view.h \
    View/image_video_view.h \
    View/login_view.h \
    View/main_view.h \
    View/monitor_view.h \
    View/normal_video_view.h \
    View/setting_view.h \
    View/imagecanvas.h \
    Yolo/Detect.h \
    Yolo/YOLO.h \
    util/dbconnectionpool.h \
    util/mysqlconnect.h \
    util/winmanage.h \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DESTDIR = $$PWD/bin

OPENCV_ROOT = $$PWD/third_libary/opencv
INCLUDEPATH += $$PWD/third_libary/opencv/include
LIBS = -L$$PWD/third_libary/opencv/x64/mingw/lib
LIBS += -llibopencv_core4100
LIBS += -llibopencv_imgproc4100
LIBS += -llibopencv_videoio4100
LIBS += -llibopencv_highgui4100
LIBS += -llibopencv_imgcodecs4100
LIBS += -llibopencv_dnn4100
LIBS += -llibopencv_features2d4100
LIBS += -llibopencv_calib3d4100
LIBS += -llibopencv_objdetect4100

RESOURCES += \
    Resources.qrc

DISTFILES +=
TARGET=MonitorSystem        #exe程序的前缀名