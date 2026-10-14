#include "mysqlconnect.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

mySqlconnect *mySqlconnect::one_instance = nullptr;
mySqlconnect *mySqlconnect::getOne_instance()
{

    if (!one_instance) {
        one_instance = new mySqlconnect;
    }
    return one_instance;
}

mySqlconnect::mySqlconnect()
{
    const QString path = QDir(QCoreApplication::applicationDirPath())
    .filePath("db_config.ini");
    //数据库INI配置文件，与exe程序同级目录
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("无法读取数据库配置文件：\n%1\n%2")
                    .arg(path, file.errorString());
        return;
    }
    file.close();
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("Database");
    //INI配置文件的键
    const QStringList keys = {"hostName", "databaseName", "userName", "password", "port"};
    for (const QString &key : keys) {
        if (!settings.contains(key)) {
            error = QStringLiteral("配置文件缺少 [Database] 下的 %1：\n%2").arg(key, path);
            return;
        }
    }
    const QString host = settings.value("hostName").toString().trimmed();
    const QString name = settings.value("databaseName").toString().trimmed();
    const QString user = settings.value("userName").toString().trimmed();
    const QString password = settings.value("password").toString();
    bool validPort = false;
    const int port = settings.value("port").toString().toInt(&validPort);
    settings.endGroup();
    if (settings.status() != QSettings::NoError || host.isEmpty() ||
        name.isEmpty() || user.isEmpty() || !validPort || port < 1 || port > 65535) {
        error = QStringLiteral("数据库配置格式或参数无效：\n%1\n请检查主机、库名、用户名和端口。")
                    .arg(path);
        return;
    }
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        error = QStringLiteral("QMYSQL 驱动不可用。请检查 sqldrivers/qsqlmysql.dll 及其依赖。\n配置文件：%1\n可用驱动：%2")
                    .arg(path, QSqlDatabase::drivers().join(", "));
        return;
    }
    //添加、设置数据库
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(host);
    db.setDatabaseName(name);
    db.setUserName(user);
    db.setPassword(password);
    db.setPort(port);
    db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=5");
    if (!db.open()) {
        error = QStringLiteral("数据库连接失败。\n配置文件：%1\n服务器：%2:%3\n数据库：%4\n用户：%5\n原因：%6")
                    .arg(path).arg(host).arg(port).arg(name).arg(user).arg(db.lastError().text());
        return;
    }
    const QStringList checks = {
        "SELECT Id,Username,Password_user,status_user,login_fail_count,lock_until FROM user_t LIMIT 0",
        "SELECT Id,Channel_id,Camera_index,Cameraname,status_camera FROM camera_t LIMIT 0",
        "SELECT Id,Camera_id,file_size,Videoname,Video_path,start_time,end_time,create_time FROM video_t LIMIT 0",
        "SELECT Id,Camera_id,file_size,Videoname,Video_path,start_time,end_time,create_time FROM detect_t LIMIT 0",
        "SELECT Id,Camera_id,Video_id,Imagename,Image_size,Image_path,Create_time,event_type FROM image_t LIMIT 0",
        "SELECT Id,camera_id,video_id,event_type,triger_time,image_count,description,create_time FROM exception_event LIMIT 0"
    };
    for (const QString &sql : checks) {
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            error = QStringLiteral("数据库已连接，但项目表结构检查失败。\n数据库：%1\n检查语句：%2\n原因：%3\n请确认已导入配套 SQL，且账号有访问权限。")
                        .arg(name, sql, query.lastError().text());
            return;
        }
    }
    ready = true;
}

mySqlconnect::~mySqlconnect()
{
    if (db.isValid()) {
        const QString name = db.connectionName();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }
}
QSqlDatabase mySqlconnect::getDb() const {
    return db;
}
bool mySqlconnect::isReady() const {
    return ready && db.isOpen();
}
QString mySqlconnect::lastError() const {
    return error;
}
void mySqlconnect::release() {
    delete one_instance; one_instance = nullptr;
}
QString mySqlconnect::Getmd5(const QString &str)
{
    return QString::fromLatin1(QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex());
}
bool mySqlconnect::authenticate(const QString &username, const QString &password,
                                QString &canonicalName, QString &message)
{
    Q_UNUSED(username)
    Q_UNUSED(password)
    canonicalName.clear();
    message = QStringLiteral("请通过 logincontrol 执行登录验证。");
    return false;
}
