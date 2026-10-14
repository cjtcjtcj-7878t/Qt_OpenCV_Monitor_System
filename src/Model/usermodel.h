#ifndef USERMODEL_H
#define USERMODEL_H
#include<QString>
#include<QDebug>
#include<QSqlQuery>
#include<QSqlError>
#include<QVariant>
#include"../util/mysqlconnect.h"
//登录信息的数据结构
struct LoginResult{
    bool success=false;
    //是否成功登录的标志，默认为false
    QString username;
    //输入的账户
    QString message;
    //输入的密码
};

class usermodel
{
public:
    usermodel();
    LoginResult authenticate(const QString &username,const QString &password);
    //从数据库查询登录的用户是否存在
};

#endif // USERMODEL_H
