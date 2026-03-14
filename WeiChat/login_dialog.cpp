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

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , currentConfig(ServerConfig::load())
{
    setWindowTitle(QStringLiteral("登录 EasyChat"));
    setModal(true);
    resize(420, 320);

    auto *layout = new QVBoxLayout(this);
    auto *titleLabel = new QLabel(QStringLiteral("请输入账号信息"), this);
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
    loginButton = new QPushButton(QStringLiteral("登录"), this);
    auto *cancelButton = new QPushButton(QStringLiteral("取消"), this);

    passwordEdit->setEchoMode(QLineEdit::Password);
    loginPortEdit->setValidator(new QIntValidator(1, 65535, loginPortEdit));
    registerPortEdit->setValidator(new QIntValidator(1, 65535, registerPortEdit));
    chatPortEdit->setValidator(new QIntValidator(1, 65535, chatPortEdit));
    friendPortEdit->setValidator(new QIntValidator(1, 65535, friendPortEdit));
    offlinePortEdit->setValidator(new QIntValidator(1, 65535, offlinePortEdit));
    statusLabel->setStyleSheet(QStringLiteral("color:#c0392b;"));

    formLayout->addRow(QStringLiteral("用户名"), usernameEdit);
    formLayout->addRow(QStringLiteral("密码"), passwordEdit);
    formLayout->addRow(QStringLiteral("服务器"), hostEdit);
    formLayout->addRow(QStringLiteral("登录端口"), loginPortEdit);
    formLayout->addRow(QStringLiteral("注册端口"), registerPortEdit);
    formLayout->addRow(QStringLiteral("聊天端口"), chatPortEdit);
    formLayout->addRow(QStringLiteral("好友端口"), friendPortEdit);
    formLayout->addRow(QStringLiteral("下线端口"), offlinePortEdit);

    hostEdit->setPlaceholderText(QStringLiteral("输入 IPv6 地址或域名，例如 240a:... 或 chat.example.com"));

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(loginButton);

    layout->addWidget(titleLabel);
    layout->addLayout(formLayout);
    layout->addWidget(statusLabel);
    layout->addStretch();
    layout->addLayout(buttonLayout);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    connect(cancelButton, &QPushButton::clicked, this, &LoginDialog::reject);
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