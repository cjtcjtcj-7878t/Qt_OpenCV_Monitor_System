#ifndef LOGIN_VIEW_H
#define LOGIN_VIEW_H

#include <QWidget>
#include<QHBoxLayout>
#include<QVBoxLayout>
#include<QLabel>
#include<QLineEdit>
#include<QPushButton>
#include<QString>
#include<QFile>
#include<QMouseEvent>
#include<QEvent>
#include<QMessageBox>
#include<QPoint>
#include<QShowEvent>
#include"captchalable.h"
class login_view : public QWidget
{
    Q_OBJECT
public:
    explicit login_view(QWidget *parent = nullptr);
    void init_qss();
    void init_connect();
    void init_widget();
    void showWarningMessage(const QString &title,const QString &text);

private:
    QLabel *Title_label;    //界面标题

    QLabel *User_label;     //账户
    QLineEdit *User_edit;       //账户输入框

    QLabel *Pwd_label;      //密码
    QLineEdit *Pwd_edit;    //密码输入框

    captchalable *code;     //验证码
    QLineEdit *edit_code;       //验证码输入框
    QLabel *code_label;     //验证码标签
    bool is_pwd_visible=false;      //密码是否可见的标志，默认为false,不可见
    QPushButton *pwd_visible;       //按钮转换密码明文或密文

    QPushButton *login_btn;     //登录按钮
    QPushButton *cancel_btn;        //取消按钮
    bool is_dragging=false;      //窗口是否处于拖动的标志
    QPoint drag_position;
protected:
    //重写函数，用于鼠标拖动窗口
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    //重写窗口画面显示函数，便于退出登录再进入登录界面的正常显示
    void showEvent(QShowEvent *event);
signals:
    void loginRequested(const QString &username,const QString &password);
public slots:
    void onLoginFailed(const QString &message);     //登录失败
};

#endif // LOGIN_VIEW_H
