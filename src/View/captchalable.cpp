#include "captchalable.h"



#include <QTime>
#include <QtGlobal>
#include <QFont>

captchalable::captchalable(QWidget *parent)
    : QLabel(parent)
{
    //初始化随机数
    // qsrand(QTime::currentTime().msec());

    setFixedSize(120,40);

    setAlignment(Qt::AlignCenter);  //设置文字居中

    setFont(QFont("Arial",18,QFont::Bold));

    setStyleSheet(
        "QLabel{"
        "background:white;"
        "border:1px solid gray;"
        "color:black;"
        "}"
        );

    refreshCode();
}

QString captchalable::createCode()
{
    QString str="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    QString result;

    for(int i=0;i<4;i++)
    {
        int index=randomInt()%str.length();

        result+=str[index];
    }

    return result;
}

int captchalable::randomInt() const
{
    //随机数函数
    static std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> dist(0,RAND_MAX);

    return dist(engine);

}

void captchalable::refreshCode()
{
    code=createCode();

    setText(code);
}

QString captchalable::getCode() const
{
    return code;
}

void captchalable::mousePressEvent(QMouseEvent *event)
{
    //表示当前不使用参数，避免编译不通过
    Q_UNUSED(event);

    refreshCode();

    emit clicked();
}
