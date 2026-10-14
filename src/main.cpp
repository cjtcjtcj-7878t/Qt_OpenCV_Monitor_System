
#include <QApplication>
#include <QMessageBox>
#include<opencv2/opencv.hpp>
#include <QSplashScreen>
#include <QPixmap>
#include "util/mysqlconnect.h"
#include<QDebug>
#include<opencv2/opencv.hpp>
#include"View/setting_view.h"
#include"View/main_view.h"
#include"View/login_view.h"
#include"Model/settinginfo.h"
#include"util/winmanage.h"
#include"../Yolo/Detect.h"
#include"Service/logincontrol.h"
#include"../util/dbconnectionpool.h"
#include"util/dbconnectionpool.h"
#include"util/mysqlconnect.h"
#include"util/winmanage.h"

void test_Yolo_image(){
    //测试图片+物体检测
    //Yolo模型检测测试

    std::string path="data/model/yolov8n.onnx";
    //加载Yolo模型
    Detect detect(path);
    cv::Mat image=cv::imread("data/images/bus.jpg");

    if(image.empty()){
        qDebug()<<"待检测图片为空";
        return;
    }

    //获取模型尺寸
    detect.model_input_size=cv::Size(640,640);

    //定义输出检测数据容器
    std::vector<YOLO_OUT> yolo_out;
    //获取图片原始大小
    detect.org_Size=image.size();
    //图像预处理，将图片转为640*640
    cv::Mat out_frame=detect.letterbox(image,detect.model_input_size.width,detect.model_input_size.height,cv::Scalar());
    //模型推理，获取检测到的数据
    detect.detect(out_frame,yolo_out);
    //绘制检测框并标记物体名称
    detect.draw(image,yolo_out);
    cv::imshow("Detect image",image);
    cv::waitKey(0);

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //先连接数据库
    auto *database=mySqlconnect::getOne_instance();
    if(!database->isReady()){
        QMessageBox::critical(nullptr,"数据库连接失败!",database->lastError());
        mySqlconnect::release();
        return -1;
    }


    //初始化数据库连接池
    dbconnectionpool::getOne_instance();

    //是否第一次进行配置操作的标志
    bool isSetted=settinginfo::LoadFromfile();
    winmanage *manage_win=winmanage::Getinstance();
    manage_win->setting_widget.Setupfirst_time(!isSetted);

    //当isSetted为true时,isfirstSetup为false;此时显示主界面
    //当isSetted为false时，isfirstSetup为true;此时显示设置界面
    if(isSetted){
        //已经进行过设置操作直接显示主画面
        manage_win->main_widget.show();
    }else{
        //未进行设置操作显示设置界面
        manage_win->setting_widget.show();
    }

    logincontrol loginController;
    //全生命周期的登录
    // View → Controller：请求登录
    QObject::connect(&manage_win->login_widget,&login_view::loginRequested,&loginController,&logincontrol::login);

    // Controller → View：登录失败，清除所有输入框内容并刷新验证码
    QObject::connect(&loginController,&logincontrol::loginFailed,&manage_win->login_widget,&login_view::onLoginFailed);

    // Controller → View：登录成功，显示主界面，隐藏登录界面，显示登陆的账号名
    QObject::connect(&loginController,&logincontrol::loginSucceeded,&manage_win->main_widget,
                     [manage_win](const QString &username) {
                         // 修改按钮文字并显示主界面
                         manage_win->main_widget.showUsername(username);
                         manage_win->login_widget.hide();
                     });

    //logincontroller接收accountButtonclicked()再根据当前登录状态进行相应操作
    QObject::connect(
        &manage_win->main_widget,
        &main_view::accountButtonClicked,
        &loginController,
        [manage_win, &loginController]() {
            if (loginController.isLoggedIn()) {
                // 已登录：查看账号菜单
                manage_win->main_widget.showAccountMenu(loginController.currentUsername(),loginController.currentLoginTime());
            } else {
                // 未登录：显示登录界面，隐藏主界面
                manage_win->login_widget.show();
                manage_win->main_widget.hide();
            }
        }
        );
    //点击退出登录按钮,通过logincontrol发出退出登录信号
    QObject::connect(
        &manage_win->main_widget,
        &main_view::logoutRequested,
        &loginController,
        &logincontrol::logout
        );
    //退出登录后，main_view显示退出登录后该有的样子
    QObject::connect(
        &loginController,
        &logincontrol::loggedOut,
        &manage_win->main_widget,
        &main_view::showLoggedOut
        );
    // test_Yolo_image();
    const int result=QApplication::exec();
    dbconnectionpool::getOne_instance()->releaseConnection();
    mySqlconnect::release();
    return result;
}

