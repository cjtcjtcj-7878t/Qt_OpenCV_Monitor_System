#ifndef DBCONNECTIONPOOL_H
#define DBCONNECTIONPOOL_H


#include<QSqlDatabase>
#include<QMutex>
#include<QMutexLocker>
#include<QHash>
#include<QString>
#include<QList>
#include<QSqlQuery>
#include<QThread>
#include<QSettings>
#include<QUuid>
#include<QDebug>
class dbconnectionpool
{
public:
    //懒汉式单例模式(不一定非得是单例模式，多个连接池情况下不用单例模式)

    static dbconnectionpool *getOne_instance();
    void releaseConnection();       //释放连接
    QSqlDatabase Getconnection();       //获取当前线程可用连接
private:
    static dbconnectionpool *one_instance;
    dbconnectionpool();
    ~dbconnectionpool();    //私有化构造函数与析构函数，防止外界构造

    //
    dbconnectionpool &operator=(const dbconnectionpool &)=delete;
    dbconnectionpool(const dbconnectionpool &)=delete;

    bool isConnectionValid(const QString &name);

    QMutex mutex;   //互斥锁，保护链接队列
    QHash<Qt::HANDLE,QString> threadcurrentcon;     //保存连接的容器
    QString hostname;   //数据库主机名
    QString databaseName;   //数据库名称
    QString userName;   //连接账号名
    QString passWord;   //连接密码
    int port;   //连接数据库的端口号
};

#endif // DBCONNECTIONPOOL_H
