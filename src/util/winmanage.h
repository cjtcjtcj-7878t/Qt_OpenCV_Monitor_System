#ifndef WINMANAGE_H
#define WINMANAGE_H

#include"../View/setting_view.h"
#include"../View/login_view.h"
#include"../View/main_view.h"
class winmanage
{
    //用于界面转换
public:
    //单例模式
    static winmanage* Getinstance();
    //唯一提供外界访问的静态变量
    static winmanage *one_instance;
    //禁用拷贝构造与赋值
    winmanage &operator=(const winmanage&)=delete;
    winmanage(const winmanage&)=delete;

    //界面成员-用于独立界面切换
    Setting_view setting_widget;
    main_view main_widget;      //main_view中使用堆叠布局进行监控、普通回放、侦测回放以及图片回放功能的切换
    login_view login_widget;
private:
    //私有化构造函数与析构函数
    winmanage();
    ~winmanage();

};

#endif // WINMANAGE_H
