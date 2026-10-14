#include "logincontrol.h"

logincontrol::logincontrol(QObject *parent)
    : QObject{parent}
{}

bool logincontrol::isLoggedIn() const
{
    return loggedIn;
}

QString logincontrol::currentUsername() const
{
    return accountName;
}

QDateTime logincontrol::currentLoginTime() const
{
    return loginTime;
}



void logincontrol::login(const QString &username, const QString &password)
{
    if(username.trimmed().isEmpty()||password.isEmpty()){
        //发出登录失败的信号
        emit loginFailed("账号或密码不能为空");
        return;
    }

    LoginResult result=userModel.authenticate(username.trimmed(),password);    //执行登录逻辑
    if(result.success){
        //登录成功
        loggedIn=true;
        accountName=result.username;
        loginTime=QDateTime::currentDateTime();
        emit loginSucceeded(accountName);
    }else{
        //登录失败
        emit loginFailed(result.message);
    }
}

void logincontrol::logout()
{

    loggedIn = false;
    accountName.clear();
    loginTime = QDateTime();

    //发出退出登录的信号
    emit loggedOut();

}
