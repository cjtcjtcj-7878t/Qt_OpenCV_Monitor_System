#ifndef CAPTCHALABLE_H
#define CAPTCHALABLE_H

#include <QLabel>
#include<QFont>
#include<QMutex>
#include<QMutexLocker>
#include<QString>
#include<QMouseEvent>
#include<random>
using namespace std;
class captchalable : public QLabel
{
    Q_OBJECT
public:
     explicit captchalable(QWidget *parent = NULL);
    //获取验证码
    QString getCode() const;    //获取当前显示的验证码

public slots:

    //刷新验证码
    void refreshCode(); //生成新验证码

signals:

    //点击验证码时发射点击信号
    void clicked();

protected:

    void mousePressEvent(QMouseEvent *event);

private:

    QString code;   //保存当前显示的验证码

    QString createCode();   //生成验证码
    int randomInt() const;  //生成随机数
};

#endif // CAPTCHALABLE_H
