#include "login_view.h"
#include"../util/winmanage.h"
login_view::login_view(QWidget *parent)
    : QWidget{parent}
{
    this->init_widget();
    this->init_qss();
    this->init_connect();

}

void login_view::init_qss()
{
    //调用qss文件进行界面优化
    QFile file(":/qss/login_qss");
    if(file.open(QFile::ReadOnly)){
        this->setStyleSheet(file.readAll());
        file.close();
    }
}

void login_view::init_connect()
{
    //密码明文、密文转换
    connect(pwd_visible, &QPushButton::clicked,this, [this]()
            {
                is_pwd_visible = !is_pwd_visible;
                if(is_pwd_visible)
                {
                    // 显示明文
                    Pwd_edit->setEchoMode(QLineEdit::Normal);
                    pwd_visible->setText("隐藏");
                    //转换为密文提示
                }
                else
                {
                    // 显示密文
                    Pwd_edit->setEchoMode(QLineEdit::Password);
                    pwd_visible->setText("显示");
                    //转换为明文提示
                }
            });

    //点击"取消"按钮回到主界面
    connect(cancel_btn,&QPushButton::clicked,this,[this](){
        this->hide();
        winmanage::Getinstance()->main_widget.show();
    });

    //点击登录
    connect(login_btn,&QPushButton::clicked,this,[this](){

        //用户名和验证码允许忽略首尾空格
        QString username = User_edit->text().trimmed();
        //密码按原样检验
        QString password = Pwd_edit->text();
        QString inputCode = edit_code->text().trimmed();

        // 用户名
        if(username.isEmpty())
        {
            showWarningMessage("登录失败", "请输入用户名");
            User_edit->setFocus();
            return;
        }

        // 密码
        if(password.isEmpty())
        {
            showWarningMessage("登录失败", "请输入密码");
            Pwd_edit->setFocus();
            return;
        }

        // 验证码
        if(inputCode.isEmpty())
        {
            showWarningMessage("登录失败", "请输入验证码");
            edit_code->setFocus();
            return;
        }

        // 验证码是否正确（不区分英文的大小写）
        if(inputCode.compare(code->getCode(),Qt::CaseInsensitive) != 0)
        {
            //不区分英文大小写
            showWarningMessage("登录失败","验证码错误，请重新输入");
            edit_code->clear();

            // 生成新的验证码
            code->refreshCode();

            //光标聚焦验证码框
            edit_code->setFocus();

            return;
        }

        // ===========================
        // 到这里代表验证码验证成功
        // ===========================

        qDebug() << "用户名:" << username;
        qDebug() << "验证码正确";

        // 后续检查用户名和密码
       emit loginRequested(username, password);
    });

}

void login_view::init_widget()
{
    // this->setFixedSize(800,600);
    // //界面主布局
    // QVBoxLayout *main_layout=new QVBoxLayout;
    // this->setLayout(main_layout);
    // this->setWindowFlag(Qt::FramelessWindowHint);
    // main_layout->setSpacing(20);
    // this->setContentsMargins(100,10,100,10);
    // main_layout->addStretch(1);

    // //标题设置
    // Title_label=new QLabel("登录");
    // Title_label->setAlignment(Qt::AlignCenter);
    // QHBoxLayout *title_layout=new QHBoxLayout;
    // title_layout->addWidget(Title_label);
    // main_layout->addLayout(title_layout);
    // main_layout->addStretch(1);

    // //账户输入栏
    // User_label=new QLabel("账户名:");
    // User_label->setAlignment(Qt::AlignCenter);
    // User_edit=new QLineEdit;
    // QHBoxLayout *user_layout=new QHBoxLayout;
    // user_layout->addWidget(User_label,2);
    // user_layout->addWidget(User_edit,8);
    // main_layout->addLayout(user_layout);
    // main_layout->addStretch(1);

    // //密码输入栏
    // Pwd_label=new QLabel("密码:");
    // Pwd_label->setAlignment(Qt::AlignCenter);
    // Pwd_edit=new QLineEdit;
    // QHBoxLayout *pwd_layout=new QHBoxLayout;
    // pwd_layout->addWidget(Pwd_label,2);
    // pwd_layout->addWidget(Pwd_edit,8);
    // main_layout->addLayout(pwd_layout);
    // main_layout->addStretch(1);

    // //验证码布局控件
    // QHBoxLayout *code_layout=new QHBoxLayout;
    // code=new captchalable;
    // QLabel *code_ver=new QLabel("验证码:");
    // QLabel *label_code=new QLabel("请输入验证码");
    // edit_code=new QLineEdit;
    // edit_code->setMaximumWidth(90);
    // edit_code->setMinimumWidth(50);
    // edit_code->setMaxLength(4);
    // code_layout->addWidget(code_ver,1);
    // code_layout->addWidget(edit_code,5);
    // main_layout->addLayout(code_layout);


    // //登录、取消按钮
    // login_btn=new QPushButton("登录");
    // cancel_btn=new QPushButton("取消");
    // QHBoxLayout *btn_layout=new QHBoxLayout;
    // btn_layout->addWidget(login_btn);
    // btn_layout->addWidget(cancel_btn);
    // main_layout->addLayout(btn_layout);


        this->resize(900, 600);
        this->setMinimumSize(800, 550);
        this->setWindowFlag(Qt::FramelessWindowHint);
        this->setObjectName("loginWidget");

        // =========================
        // 整个窗口主布局
        // =========================
        QVBoxLayout *main_layout = new QVBoxLayout(this);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);


        // =========================
        // 顶部标题栏
        // =========================
        QWidget *titleBar = new QWidget(this);
        titleBar->setObjectName("titleBar");
        titleBar->setFixedHeight(50);

        QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
        titleBarLayout->setContentsMargins(20, 0, 0, 0);
        titleBarLayout->setSpacing(0);

        QLabel *windowTitle = new QLabel("智能监控系统");
        windowTitle->setObjectName("windowTitle");

        QPushButton *minBtn = new QPushButton("—");
        QPushButton *closeBtn = new QPushButton("×");

        minBtn->setObjectName("windowButton");
        closeBtn->setObjectName("closeButton");

        minBtn->setFixedSize(46, 50);
        closeBtn->setFixedSize(46, 50);

        titleBarLayout->addWidget(windowTitle);
        titleBarLayout->addStretch();
        titleBarLayout->addWidget(minBtn);
        titleBarLayout->addWidget(closeBtn);

        main_layout->addWidget(titleBar);


        // =========================
        // 中央区域
        // =========================
        QHBoxLayout *centerLayout = new QHBoxLayout;
        centerLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *loginCard = new QWidget;
        loginCard->setObjectName("loginCard");
        loginCard->setFixedSize(380, 440);

        QVBoxLayout *cardLayout = new QVBoxLayout(loginCard);
        cardLayout->setContentsMargins(40, 35, 40, 35);
        cardLayout->setSpacing(12);


        // =========================
        // 标题
        // =========================
        Title_label = new QLabel("智能监控系统");
        Title_label->setObjectName("loginTitle");
        Title_label->setAlignment(Qt::AlignCenter);

        QLabel *subTitle = new QLabel("登录以进入监控管理平台");
        subTitle->setObjectName("loginSubTitle");
        subTitle->setAlignment(Qt::AlignCenter);

        cardLayout->addWidget(Title_label);
        cardLayout->addWidget(subTitle);

        cardLayout->addSpacing(20);


        // =========================
        // 用户名
        // =========================
        User_label = new QLabel("用户名");
        User_label->setObjectName("formLabel");

        User_edit = new QLineEdit;
        User_edit->setObjectName("loginEdit");
        User_edit->setPlaceholderText("请输入用户名");    //未输入内容时的输入框提示
        User_edit->setClearButtonEnabled(true);     //设置一键清空按钮
        User_edit->setFixedHeight(42);      //最大高度
        User_edit->setMaxLength(10);        //账户最大长度为10
        cardLayout->addWidget(User_label);
        cardLayout->addWidget(User_edit);

        cardLayout->addSpacing(8);


        // =========================
        // 密码
        // =========================
        Pwd_label = new QLabel("密码");
        Pwd_label->setObjectName("formLabel");

        Pwd_edit = new QLineEdit;


        Pwd_edit->setObjectName("loginEdit");
        Pwd_edit->setPlaceholderText("请输入密码");
        Pwd_edit->setMaxLength(8);      //密码最大长度为8
        // 默认密码隐藏
        Pwd_edit->setEchoMode(QLineEdit::Password);     //密码显示格式：星号或圆点
        Pwd_edit->setFixedHeight(42);

        pwd_visible = new QPushButton("显示");
        pwd_visible->setObjectName("pwdToggleButton");
        pwd_visible->setFixedSize(60, 42);

        QHBoxLayout *pwdInputLayout = new QHBoxLayout;
        pwdInputLayout->setSpacing(8);
        pwdInputLayout->addWidget(Pwd_edit);

        pwdInputLayout->addWidget(pwd_visible);
        cardLayout->addWidget(Pwd_label);

        cardLayout->addLayout(pwdInputLayout);

        //验证码
        code_label = new QLabel("验证码");
        code_label->setObjectName("formLabel");

        edit_code = new QLineEdit;
        edit_code->setObjectName("loginEdit");
        edit_code->setPlaceholderText("请输入验证码");
        edit_code->setFixedHeight(42);
        edit_code->setMaxLength(4);     //最多4位数字
        code = new captchalable;
        code->setObjectName("captchaLabel");
        code->setFixedSize(120, 42);

        QHBoxLayout *captchaLayout = new QHBoxLayout;
        captchaLayout->setSpacing(8);

        captchaLayout->addWidget(edit_code);
        captchaLayout->addWidget(code);
        cardLayout->addWidget(code_label);
        cardLayout->addLayout(captchaLayout);

        // =========================
        // 登录、取消按钮
        // =========================
        login_btn = new QPushButton("登录");
        login_btn->setObjectName("loginButton");
        login_btn->setFixedHeight(44);

        cancel_btn = new QPushButton("取消");
        cancel_btn->setObjectName("cancelButton");
        cancel_btn->setFixedHeight(38);

        cardLayout->addWidget(login_btn);
        cardLayout->addWidget(cancel_btn);


        // 卡片整体居中
        centerLayout->addStretch();
        centerLayout->addWidget(loginCard);
        centerLayout->addStretch();

        main_layout->addStretch();
        main_layout->addLayout(centerLayout);
        main_layout->addStretch();


        // =========================
        // 窗口按钮
        // =========================
        connect(minBtn, &QPushButton::clicked,
                this, &QWidget::showMinimized);

        connect(closeBtn, &QPushButton::clicked,
                this, [this](){
            this->hide();
            winmanage::Getinstance()->main_widget.show();
        });



}

void login_view::showWarningMessage(const QString &title, const QString &text)
{
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::NoIcon);
    msg.setWindowTitle(title);
    msg.setText(text);
    msg.setStandardButtons(QMessageBox::Ok);

    // 找到 QMessageBox 内部显示文本的 QLabel
    QLabel *label = msg.findChild<QLabel*>("qt_msgbox_label");
    if(label)
    {
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setMinimumWidth(280);
    }

    msg.exec();
}

void login_view::mouseMoveEvent(QMouseEvent *event)
{
    if(is_dragging&&(event->buttons()&Qt::LeftButton)){
        this->move(event->globalPosition().toPoint()-drag_position);
        event->accept();
    }
}

void login_view::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        is_dragging=true;
        drag_position=event->globalPosition().toPoint()-this->frameGeometry().topLeft();
        event->accept();
    }
}

void login_view::mouseReleaseEvent(QMouseEvent *event)
{
    is_dragging=false;
    event->accept();
}

void login_view::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 清空账号、密码和验证码输入
    User_edit->clear();
    Pwd_edit->clear();
    edit_code->clear();

    // 密码恢复隐藏状态
    is_pwd_visible = false;
    Pwd_edit->setEchoMode(QLineEdit::Password);
    pwd_visible->setText("显示");

    // 重新生成验证码
    code->refreshCode();

    // 光标回到账户输入框
    User_edit->setFocus();
}

void login_view::onLoginFailed(const QString &message)
{
    showWarningMessage("登录失败", message);    //显示错误信息

    Pwd_edit->clear();      //清空密码输入框
    edit_code->clear();     //清空验证码输入框
    code->refreshCode();        //刷新验证码

    Pwd_edit->setFocus();   //光标重新回到密码输入框
}
