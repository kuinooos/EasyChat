#include "login_dialog.h"

#include <QDataStream>
#include <QFormLayout>
#include <QHostAddress>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QVBoxLayout>
#include "animatediconbutton.h"
#include "titlebar.h"
#include <QApplication>
#include <QStyle>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , currentConfig(ServerConfig::load())
{
    setWindowTitle(QStringLiteral("登录 EasyChat"));
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setObjectName("app_root");
    setModal(true);
    resize(420, 320);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    auto *titleBar = new TitleBar(this);
    titleBar->setTitle(QStringLiteral("登录 EasyChat"));
    if (auto *maxBtn = titleBar->findChild<AnimatedIconButton *>("title_max_btn")) {
        maxBtn->hide();
    }

    auto *titleLabel = new QLabel(QStringLiteral("请输入账号信息"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    auto *formLayout = new QFormLayout();

    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    hostEdit = new QLineEdit(currentConfig.host, this);
    loginPortEdit = new QLineEdit(QString::number(currentConfig.loginPort), this);
    registerPortEdit = new QLineEdit(QString::number(currentConfig.registerPort), this);
    chatPortEdit = new QLineEdit(QString::number(currentConfig.chatPort), this);
    friendPortEdit = new QLineEdit(QString::number(currentConfig.friendPort), this);
    offlinePortEdit = new QLineEdit(QString::number(currentConfig.offlinePort), this);
    statusLabel = new QLabel(this);
    loginButton = new AnimatedIconButton(this);
    loginButton->setObjectName("login_btn");
    static_cast<AnimatedIconButton *>(loginButton)->setSvgIcon(":/svg/send.svg");
    loginButton->setToolTip(QStringLiteral("登录"));
    loginButton->setFixedSize(36, 32);

    auto *cancelButton = new AnimatedIconButton(this);
    cancelButton->setObjectName("cancel_btn");
    cancelButton->setSvgIcon(":/svg/close.svg");
    cancelButton->setToolTip(QStringLiteral("取消"));
    cancelButton->setFixedSize(36, 32);

    passwordEdit->setEchoMode(QLineEdit::Password);
    loginPortEdit->setValidator(new QIntValidator(1, 65535, loginPortEdit));
    registerPortEdit->setValidator(new QIntValidator(1, 65535, registerPortEdit));
    chatPortEdit->setValidator(new QIntValidator(1, 65535, chatPortEdit));
    friendPortEdit->setValidator(new QIntValidator(1, 65535, friendPortEdit));
    offlinePortEdit->setValidator(new QIntValidator(1, 65535, offlinePortEdit));
    statusLabel->setStyleSheet(QStringLiteral("color:#c0392b;"));

    QFont labelFont = font();
    labelFont.setPointSize(13);
    QFont inputFont = font();
    inputFont.setPointSize(14);
    usernameEdit->setFont(inputFont);
    passwordEdit->setFont(inputFont);
    hostEdit->setFont(inputFont);
    loginPortEdit->setFont(inputFont);
    registerPortEdit->setFont(inputFont);
    chatPortEdit->setFont(inputFont);
    friendPortEdit->setFont(inputFont);
    offlinePortEdit->setFont(inputFont);

    formLayout->addRow(QStringLiteral("用户名"), usernameEdit);
    formLayout->addRow(QStringLiteral("密码"), passwordEdit);
    formLayout->addRow(QStringLiteral("服务器"), hostEdit);
    formLayout->addRow(QStringLiteral("登录端口"), loginPortEdit);
    formLayout->addRow(QStringLiteral("注册端口"), registerPortEdit);
    formLayout->addRow(QStringLiteral("聊天端口"), chatPortEdit);
    formLayout->addRow(QStringLiteral("好友端口"), friendPortEdit);
    formLayout->addRow(QStringLiteral("下线端口"), offlinePortEdit);

    for (auto *label : findChildren<QLabel*>()) {
        if (label != titleLabel) {
            label->setFont(labelFont);
        }
    }

    hostEdit->setPlaceholderText(QStringLiteral("输入 IPv6 地址或域名，例如 240a:... 或 chat.example.com"));

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(loginButton);

    layout->addWidget(titleBar);
    layout->addWidget(titleLabel);
    layout->addLayout(formLayout);
    layout->addWidget(statusLabel);
    layout->addStretch();
    layout->addLayout(buttonLayout);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    connect(cancelButton, &QPushButton::clicked, this, &LoginDialog::reject);
    connect(titleBar, &TitleBar::sigClose, this, &LoginDialog::reject);
    connect(titleBar, &TitleBar::sigMinimize, this, &LoginDialog::showMinimized);
    connect(titleBar, &TitleBar::sigToggleTheme, this, [this]() {
        const QString current = property("theme").toString().isEmpty() ? QStringLiteral("dark") : property("theme").toString();
        const QString next = (current == QStringLiteral("dark")) ? QStringLiteral("light") : QStringLiteral("dark");
        setProperty("theme", next);
        qApp->setProperty("theme", next);
        const bool light = (next == QStringLiteral("light"));
        const QColor base = light ? QColor(30, 31, 33, 220) : QColor(220, 223, 228, 220);
        const QColor hover = light ? QColor(10, 10, 12, 255) : QColor(255, 255, 255, 230);
        const QColor press = light ? QColor(0, 0, 0, 200) : QColor(255, 255, 255, 200);
        const QString iconPath = light ? QStringLiteral(":/svg/sun.svg") : QStringLiteral(":/svg/moon.svg");

        for (auto *btn : findChildren<AnimatedIconButton*>()) {
            btn->setBaseColor(base);
            btn->setHoverColor(hover);
            btn->setPressColor(press);
            if (btn->objectName() == QStringLiteral("title_theme_btn")) {
                btn->setSvgIcon(iconPath);
            }
        }
        style()->unpolish(this);
        style()->polish(this);
        update();
    });

    setProperty("theme", QStringLiteral("dark"));
}

QString LoginDialog::username() const
{
    return usernameEdit->text().trimmed();
}

QString LoginDialog::serverHost() const
{
    return hostEdit->text().trimmed();
}

quint16 LoginDialog::loginPort() const
{
    return readPort(loginPortEdit, currentConfig.loginPort);
}

ServerConfig LoginDialog::config() const
{
    ServerConfig config = currentConfig;
    config.host = serverHost();
    config.loginPort = readPort(loginPortEdit, config.loginPort);
    config.registerPort = readPort(registerPortEdit, config.registerPort);
    config.chatPort = readPort(chatPortEdit, config.chatPort);
    config.friendPort = readPort(friendPortEdit, config.friendPort);
    config.offlinePort = readPort(offlinePortEdit, config.offlinePort);
    return config;
}

quint16 LoginDialog::readPort(QLineEdit *edit, quint16 fallback) const
{
    bool ok = false;
    const quint16 port = static_cast<quint16>(edit->text().toUShort(&ok));
    return ok && port != 0 ? port : fallback;
}

void LoginDialog::attemptLogin()
{
    const QString currentUsername = username();
    const QString currentPassword = passwordEdit->text();
    const ServerConfig configValue = config();
    const QString host = configValue.host;
    const quint16 port = configValue.loginPort;

    if (currentUsername.isEmpty() || currentPassword.isEmpty()) {
        statusLabel->setText(QStringLiteral("用户名和密码不能为空"));
        return;
    }

    if (host.isEmpty() || port == 0) {
        statusLabel->setText(QStringLiteral("服务器地址或端口无效"));
        return;
    }

    QTcpSocket socket;
    socket.connectToHost(host, port);
    if (!socket.waitForConnected(3000)) {
        statusLabel->setText(QStringLiteral("连接登录服务器失败"));
        return;
    }

    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out << currentUsername << currentPassword;

    socket.write(payload);
    if (!socket.waitForBytesWritten(3000)) {
        statusLabel->setText(QStringLiteral("登录请求发送失败"));
        socket.disconnectFromHost();
        return;
    }

    if (!socket.waitForReadyRead(3000)) {
        statusLabel->setText(QStringLiteral("登录服务器未返回结果"));
        socket.disconnectFromHost();
        return;
    }

    const QString response = QString::fromUtf8(socket.readAll()).trimmed();
    socket.disconnectFromHost();

    if (response.compare(QStringLiteral("successful"), Qt::CaseInsensitive) == 0) {
        currentConfig = configValue;
        currentConfig.save();
        accept();
        return;
    }

    statusLabel->setText(QStringLiteral("用户名或密码错误"));
}
