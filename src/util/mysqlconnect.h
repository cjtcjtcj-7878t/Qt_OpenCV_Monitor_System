#ifndef MYSQLCONNECT_H
#define MYSQLCONNECT_H
#include <QSqlDatabase>
#include <QString>

class mySqlconnect
{
public:
    //懒汉式单例模式
    static mySqlconnect *one_instance;  //外部可访问的唯一实例
    static mySqlconnect *getOne_instance(); //外部获取唯一实例的方法

    //禁用拷贝与赋值
    mySqlconnect(const mySqlconnect &) = delete;
    mySqlconnect &operator=(const mySqlconnect &) = delete;
    QSqlDatabase getDb() const;
    static void release();
    QString Getmd5(const QString &str);
    bool isReady() const;
    QString lastError() const;
    bool authenticate(const QString &username, const QString &password,QString &canonicalName, QString &message);
private:
    //私有化构造与析构函数
    mySqlconnect();
    ~mySqlconnect();
    QSqlDatabase db;
    QString error;
    bool ready = false;
};
#endif
