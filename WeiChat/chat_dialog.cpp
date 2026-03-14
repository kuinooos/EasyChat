#include "chat_dialog.h"
#include "ui_chat_dialog.h"
#include<QRandomGenerator>

#include<QAction>
#include<QCloseEvent>
#include<QIcon>
#include<QLineEdit>
#include<QListWidgetItem>
#include<QMessageBox>
#include<QPushButton>
#include<QTextEdit>
#include<QTcpSocket>
#include<QStringList>
#include<vector>
#include<chatuserwid.h>
Chat_Dialog::Chat_Dialog(const QString &username, const ServerConfig &serverConfig, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Chat_Dialog)
    , username(username)
    , serverConfig(serverConfig)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);
    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));
    // 创建一个清除动作并设置图标
    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);
    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 文本为空时，切换回透明图标
        }
    });
    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 清除文本后，切换回透明图标
        ui->search_edit->clearFocus();
        //清除按钮被按下则不显示搜索框
        //ShowSearch(false);
    });
    ui->search_edit->SetMaxLength(15);

//    connect(ui->chat_user_list,&ChatUserList::sig_loading_chat_user,this,&Chat_Dialog::slot_loading_chat_user);//gaidon
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, &Chat_Dialog::onChatUserItemClicked);
    connect(ui->pushButton, &QPushButton::clicked, this, &Chat_Dialog::onAddFriendClicked);
    ui->pushButton->setToolTip(QStringLiteral("添加好友"));

    addChatUSerList();
    initializeConnection();
}

Chat_Dialog::~Chat_Dialog()
{
    notifyOffline();
    if (socket && socket->isOpen()) {
            socket->disconnectFromHost();
        }
    delete ui;
}

void Chat_Dialog::closeEvent(QCloseEvent *event)
{
    notifyOffline();
    QDialog::closeEvent(event);
}

std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                             "You have to love yourself",
                             "My love is written in the wind ever since the whole world is you"};

std::vector<QString> heads = {
    ":/png/xyy.jpg",
    ":/png/fyy.jpg",
    ":/png/myy.jpg",
    ":/png/lyy.jpg",
    ":/png/htl.jpg",
    ":/png/xsg.jpg",
    ":/png/hds.jpg"
};

std::vector<QString> names = {
    "喜洋洋",
    "沸羊羊",
    "美羊羊",
    "懒羊羊",
    "灰太狼",
    "潇洒哥",
    "黑大帅"
};//测试

void Chat_Dialog::initializeConnection()
{
    connect(socket, &QTcpSocket::readyRead, this, &Chat_Dialog::on_readyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Chat_Dialog::on_disconnected);

    socket->connectToHost(serverConfig.host, serverConfig.chatPort);
    if (!socket->waitForConnected(3000)) {
        if (ui->listWidget) {
            ui->listWidget->addItem(QStringLiteral("连接聊天服务器失败: ") + socket->errorString());
        }
        return;
    }

    const QString onlineMessage = QStringLiteral("ONLINE::") + username + QStringLiteral("\n");
    socket->write(onlineMessage.toUtf8());
    socket->flush();

    loadFriendList();
}




void Chat_Dialog::addChatUSerList()
{
    ui->chat_user_list->clear();

    // 如果好友服务未返回数据，则回退到演示数据
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();
        addChatUserItem(names[name_i], heads[head_i], strs[str_i], i == 0);
    }
}

void Chat_Dialog::loadFriendList()
{
    const QString response = sendFriendCommand(QStringLiteral("NOWMYFRIEND::") + username);
    if (response == QStringLiteral("__FRIEND_SERVER_ERROR__")) {
        addChatUSerList();
        if (ui->listWidget) {
            ui->listWidget->addItem(QStringLiteral("好友服务不可用，已加载演示联系人"));
        }
        return;
    }

    ui->chat_user_list->clear();
    currentPeer.clear();

    if (response.isEmpty()) {
        updateCurrentPeer(QString());
        if (ui->listWidget) {
            ui->listWidget->addItem(QStringLiteral("当前账号暂无好友"));
        }
        return;
    }

    const QStringList entries = response.split(',', Qt::SkipEmptyParts);
    int index = 0;
    for (const QString &entry : entries) {
        const QStringList fields = entry.split('+');
        const QString friendName = fields.value(0).trimmed();
        if (friendName.isEmpty()) {
            continue;
        }

        const QString status = fields.value(1).trimmed() == QStringLiteral("1")
            ? QStringLiteral("在线")
            : QStringLiteral("离线");
        const QString head = heads.at(index % static_cast<int>(heads.size()));
        addChatUserItem(friendName, head, status, index == 0);
        ++index;
    }

    if (index == 0) {
        addChatUSerList();
    }
}

QString Chat_Dialog::sendFriendCommand(const QString &command)
{
    QTcpSocket friendSocket;
    friendSocket.connectToHost(serverConfig.host, serverConfig.friendPort);
    if (!friendSocket.waitForConnected(2000)) {
        return QStringLiteral("__FRIEND_SERVER_ERROR__");
    }

    friendSocket.write(command.toUtf8());
    if (!friendSocket.waitForBytesWritten(2000)) {
        friendSocket.disconnectFromHost();
        return QStringLiteral("__FRIEND_SERVER_ERROR__");
    }

    if (!friendSocket.waitForReadyRead(2000)) {
        friendSocket.disconnectFromHost();
        return QString();
    }

    const QString response = QString::fromUtf8(friendSocket.readAll()).trimmed();
    friendSocket.disconnectFromHost();
    return response;
}

void Chat_Dialog::notifyOffline()
{
    if (offlineNotified || username.isEmpty()) {
        return;
    }

    offlineNotified = true;
    QTcpSocket offlineSocket;
    offlineSocket.connectToHost(serverConfig.host, serverConfig.offlinePort);
    if (!offlineSocket.waitForConnected(1000)) {
        return;
    }

    offlineSocket.write(username.toUtf8());
    offlineSocket.waitForBytesWritten(1000);
    offlineSocket.disconnectFromHost();
}

void Chat_Dialog::addChatUserItem(const QString &name, const QString &head, const QString &msg, bool selectByDefault)
{
    auto *chat_user_wid = new ChatUserWid();
    chat_user_wid->SetInfo(name, head, msg);
    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->addItem(item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    if (selectByDefault) {
        ui->chat_user_list->setCurrentItem(item);
        updateCurrentPeer(chat_user_wid->userName());
    }
}

void Chat_Dialog::updateCurrentPeer(const QString &peerName)
{
    currentPeer = peerName;
    if (ui->label) {
        ui->label->setText(peerName.isEmpty() ? QStringLiteral("请选择聊天对象") : peerName);
    }
}

void Chat_Dialog::onChatUserItemClicked(QListWidgetItem *item)
{
    auto *chatUser = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(item));
    if (!chatUser) {
        return;
    }
    updateCurrentPeer(chatUser->userName());
}

void Chat_Dialog::onAddFriendClicked()
{
    const QString friendName = ui->search_edit->text().trimmed();
    if (friendName.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请输入要添加的好友用户名"));
        return;
    }

    if (friendName == username) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("不能添加自己为好友"));
        return;
    }

    const QString response = sendFriendCommand(QStringLiteral("ADD::") + username + QStringLiteral("::") + friendName);
    if (response == QStringLiteral("__FRIEND_SERVER_ERROR__")) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("好友服务不可用，稍后再试"));
        return;
    }

    QMessageBox::information(this, QStringLiteral("添加好友"), response.isEmpty() ? QStringLiteral("请求已发送") : response);
    ui->search_edit->clear();
    loadFriendList();
}

void Chat_Dialog::on_chat_edit_textChanged()
{
    QString currentText = ui->chat_edit->toPlainText();

    qDebug() << "Current Text:" << currentText;

    Send_data = currentText;
}

void Chat_Dialog::on_send_btn_clicked()
{
    if (currentPeer.isEmpty()) {
           QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先在左侧选择聊天对象"));
           return;
    }

    if (socket->isOpen() && !Send_data.isEmpty()) {
           QByteArray data = (username + QStringLiteral("::") + currentPeer + QStringLiteral("::") + Send_data).toUtf8();
           socket->write(data + '\n');  // 添加换行符便于服务器解析
           ui->listWidget->addItem(QStringLiteral("Me -> ") + currentPeer + QStringLiteral(": ") + Send_data);
           ui->chat_edit->clear();
           Send_data.clear();
       }
}

void Chat_Dialog::on_readyRead()
{
    while (socket->canReadLine()) {
        QString message = socket->readLine().trimmed();
        if (message.startsWith(QStringLiteral("MESSAGE::"))) {
            const QStringList parts = message.split(QStringLiteral("::"));
            if (parts.size() >= 3) {
                const QString sender = parts.value(1);
                const QString content = parts.mid(2).join(QStringLiteral("::"));
                ui->listWidget->addItem(sender + QStringLiteral(": ") + content);
                continue;
            }
        }
        ui->listWidget->addItem(message);
    }
}

void Chat_Dialog::on_disconnected()
{
    qDebug() << "Disconnected from server!";
    ui->listWidget->addItem("Disconnected from server!");
}
