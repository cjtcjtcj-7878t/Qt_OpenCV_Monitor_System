#include "usermodel.h"

usermodel::usermodel() {}

LoginResult usermodel::authenticate(const QString &username,const QString &password)
{
    mySqlconnect *connection = mySqlconnect::getOne_instance(); //获取数据库唯一实例
    QSqlDatabase db = connection->getDb();      //获取数据库的唯一链接

    if (!db.isOpen()) {return {false, {}, "数据库未连接"};
    }

    QSqlQuery query(db);
    //执行SQL语句
    query.prepare("SELECT Username, Password_user, status_user ""FROM user_t WHERE Username = :username");

    query.bindValue(":username", username);

    if (!query.exec()) {
        qWarning() << "登录查询失败：" << query.lastError().text();
        return {false, {}, "数据库查询失败，请稍后重试"
        };
    }

    // 查询结果中没有此账号
    if (!query.next()) {
        return {false, {}, "账号或密码错误"
        };
    }

    //数据库中保存的密码
    const QString savedPassword=query.value("Password_user").toString();

    //输入的密码
    const QString inputPassword = connection->Getmd5(password);

    if (inputPassword.compare(savedPassword, Qt::CaseInsensitive) != 0) {
        return {false, {}, "账号或密码错误"
        };
    }

    if (query.value("status_user").toInt() != 1) {
        return {false, {}, "该账号已被禁用"
        };
    }

    return{true,query.value("Username").toString(),{}
        };
}
