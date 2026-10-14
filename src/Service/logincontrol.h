#ifndef LOGINCONTROL_H
#define LOGINCONTROL_H

#include <QObject>
#include<QDebug>
#include<QString>
#include<QLabel>
#include<QMessageBox>
#include<QEvent>
#include<QFile>
#include<QWidget>
#include<QPushButton>
#include<QDateTime>
#include"../Model/usermodel.h"
class logincontrol : public QObject
{
    Q_OBJECT
public:
    explicit logincontrol(QObject *parent = nullptr);
    bool isLoggedIn() const ;
    QString currentUsername() const ;   //当前已登录的用户名
    QDateTime currentLoginTime() const ;    //当前账户登录的时间
    void showWarningMessage(const QString &title,const QString &text);
public slots:
    void login(const QString &username, const QString &password);   //进行登录
    void logout();  //退出登录

signals:
    void loginSucceeded(const QString &username);   //登录成功的信号
    void loginFailed(const QString &message);       //登录失败的信号
    void loggedOut();   //退出登录的信号
private:
    usermodel userModel;    //调用Model类的方法
    bool loggedIn = false;  //是否已经登录标志，默认为false
    QString accountName;    //登录的用户名
    QDateTime loginTime;        //登录的时间
signals:
};

#endif // LOGINCONTROL_H
