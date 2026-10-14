#include "dbconnectionpool.h"
#include "mysqlconnect.h"
#include <QSqlError>

dbconnectionpool * dbconnectionpool::one_instance=nullptr;
dbconnectionpool *dbconnectionpool::getOne_instance()
{
    //外界访问的唯一接口
    static QMutex creationMutex;
    QMutexLocker locker(&creationMutex);
    if(!one_instance) one_instance=new dbconnectionpool;
    return one_instance;
}

//归还连接
void dbconnectionpool::releaseConnection()
{
    QMutexLocker locker(&mutex);
    Qt::HANDLE tid=QThread::currentThreadId();

    //连接池不包含当前连接
    if(!this->threadcurrentcon.contains(tid)){
        return;
    }

    //去除当前线程连接的数据库
    QString dataName=this->threadcurrentcon[tid];
    if(QSqlDatabase::contains(dataName)){
        QSqlDatabase::removeDatabase(dataName);
    }
    this->threadcurrentcon.remove(tid);
}

//获取连接
QSqlDatabase dbconnectionpool::Getconnection()
{
    //自动加锁/解锁，避免操作遗忘
    QMutexLocker locker(&mutex);
    //1.获取当前线程ID
    Qt::HANDLE currentthreadId=QThread::currentThreadId();
    //2、判断当前连接池是否包含当前连接,若有则检验改连接是否有效并返回
    if(this->threadcurrentcon.contains(currentthreadId)){
        QString dataname=this->threadcurrentcon[currentthreadId];
        if(isConnectionValid(dataname)){
            return QSqlDatabase::database(dataname);
        }else{
            //无效移除
            QSqlDatabase::removeDatabase(dataname);
            this->threadcurrentcon.remove(currentthreadId);
        }
    }
    //3、若无效，则建立新连接保存到threadcurrentcon容器中

    QString dataName=QUuid::createUuid().toString();
    QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL",dataName);

    //赋值
    db.setHostName(this->hostname);
    db.setDatabaseName(this->databaseName);
    db.setUserName(this->userName);
    db.setPassword(this->passWord);
    db.setPort(this->port);
    db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=3");
    if(db.open()){
        qDebug()<<"连接池已添加新连接";
        this->threadcurrentcon[currentthreadId]=dataName;
        return db;
    }
    qWarning()<<"数据库连接池连接失败："<<db.lastError().text();
    db=QSqlDatabase();
    QSqlDatabase::removeDatabase(dataName);
    return QSqlDatabase();

}

dbconnectionpool::dbconnectionpool() {
    // main() 在主线程先读取 INI 并连接成功，然后创建连接池。
    const QSqlDatabase source=mySqlconnect::getOne_instance()->getDb();
    hostname=source.hostName();
    databaseName=source.databaseName();
    userName=source.userName();
    passWord=source.password();
    port=source.port();
}

dbconnectionpool::~dbconnectionpool()
{
    QMutexLocker locker(&mutex);
    for(Qt::HANDLE tid:this->threadcurrentcon.keys()){
        QString conName=this->threadcurrentcon[tid];
        if(QSqlDatabase::contains(conName)){
            QSqlDatabase::removeDatabase(conName);
        }
    }
    this->threadcurrentcon.clear();
}

//获取连接有效性
bool dbconnectionpool::isConnectionValid(const QString &name)
{
    if(!QSqlDatabase::contains(name)){
        //不包含指定名称的连接
        return false;
    }
    //获取指定名称的连接
    QSqlDatabase db=QSqlDatabase::database(name);
    if(!db.isOpen() && !db.open()){
        return false;
    }
    //执行最简单的sql语句
    QSqlQuery query(db);
    return query.exec("select 1");
}
