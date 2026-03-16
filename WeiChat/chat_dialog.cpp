#include "chat_dialog.h"
#include "ui_chat_dialog.h"
#include "animatediconbutton.h"
#include "chatbubble.h"
#include<QRandomGenerator>

#include<QAction>
#include<QAbstractItemView>
#include<QCloseEvent>
#include<QIcon>
#include<QHBoxLayout>
#include<QLineEdit>
#include<QListWidgetItem>
#include<QMessageBox>
#include<QPushButton>
#include<QTextEdit>
#include<QTcpSocket>
#include<QStringList>
#include<QVBoxLayout>
#include<QGraphicsOpacityEffect>
#include<QGraphicsDropShadowEffect>
#include<QPropertyAnimation>
#include<QTimer>
#include<vector>
#include<chatuserwid.h>
#include<QFileDialog>
#include<QFileInfo>
#include<QDir>
#include<QMouseEvent>
#include<QUuid>
Chat_Dialog::Chat_Dialog(const QString &username, const ServerConfig &serverConfig, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Chat_Dialog)
    , username(username)
    , serverConfig(serverConfig)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);
    setObjectName("app_root");
    setAttribute(Qt::WA_StyledBackground, true);
    setupIconButtons();
    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/svg/search.svg"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));
    // 创建一个清除动作并设置图标
    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/svg/close.svg"));
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);
    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/svg/close.svg"));
        } else {
            clearAction->setIcon(QIcon(":/svg/close.svg")); // 文本为空时，切换回透明图标
        }
    });
    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/svg/close.svg")); // 清除文本后，切换回透明图标
        ui->search_edit->clearFocus();
        //清除按钮被按下则不显示搜索框
        //ShowSearch(false);
    });
    ui->search_edit->SetMaxLength(15);
    ui->listWidget->setSpacing(6);
    ui->chat_user_list->setSpacing(6);
    ui->listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (ui->chat_data_list && !ui->chat_data_list->layout()) {
        auto *layout = new QVBoxLayout(ui->chat_data_list);
        layout->setContentsMargins(16, 8, 16, 8);
        layout->setSpacing(8);
        layout->addWidget(ui->title_wid);
        layout->addWidget(ui->listWidget);
    }

    if (ui->chat_user_wid) {
        ui->chat_user_wid->setAttribute(Qt::WA_StyledBackground, true);
        auto *shadow = new QGraphicsDropShadowEffect(ui->chat_user_wid);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(0, 0, 0, 120));
        ui->chat_user_wid->setGraphicsEffect(shadow);
    }
    if (ui->chat_data_wid) {
        ui->chat_data_wid->setAttribute(Qt::WA_StyledBackground, true);
        auto *shadow = new QGraphicsDropShadowEffect(ui->chat_data_wid);
        shadow->setBlurRadius(28);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(0, 0, 0, 120));
        ui->chat_data_wid->setGraphicsEffect(shadow);
    }

//    connect(ui->chat_user_list,&ChatUserList::sig_loading_chat_user,this,&Chat_Dialog::slot_loading_chat_user);//gaidon
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, &Chat_Dialog::onChatUserItemClicked);
    if (auto *btn = findChild<AnimatedIconButton *>("add_friend_btn")) {
        connect(btn, &QPushButton::clicked, this, &Chat_Dialog::onAddFriendClicked);
        btn->setToolTip(QStringLiteral("添加好友"));
    }

    addChatUSerList();
    initializeConnection();

    if (m_fileButton) {
        connect(m_fileButton, &QPushButton::clicked, this, &Chat_Dialog::onFileClicked);
    }
}

Chat_Dialog::~Chat_Dialog()
{
    notifyOffline();
    if (socket && socket->isOpen()) {
            socket->disconnectFromHost();
        }
    delete ui;
}

void Chat_Dialog::setupIconButtons()
{
    auto replaceWithAnimated = [this](QWidget *oldWidget, const QString &objectName,
                                     const QString &iconPath, const QString &tooltip, int size) -> AnimatedIconButton * {
        if (!oldWidget) return nullptr;
        auto *btn = new AnimatedIconButton(oldWidget->parentWidget());
        btn->setObjectName(objectName);
        btn->setSvgIcon(iconPath);
        btn->setFixedSize(size, size);
        btn->setToolTip(tooltip);
        if (auto *layout = oldWidget->parentWidget()->layout()) {
            layout->replaceWidget(oldWidget, btn);
        }
        oldWidget->deleteLater();
        return btn;
    };

    replaceWithAnimated(ui->pushButton, "add_friend_btn", ":/svg/add.svg", QStringLiteral("添加好友"), 30);
    if (auto *sendBtn = replaceWithAnimated(ui->send_btn, "send_btn", ":/svg/send.svg", QStringLiteral("发送"), 36)) {
        connect(sendBtn, &QPushButton::clicked, this, &Chat_Dialog::on_send_btn_clicked);
    }

    m_fileButton = replaceWithAnimated(ui->file_lb, "file_btn", ":/svg/attach.svg", QStringLiteral("发送文件"), 28);
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
    connect(socket, &QAbstractSocket::bytesWritten, this, &Chat_Dialog::onBytesSent);

    socket->connectToHost(serverConfig.host, serverConfig.chatPort);
    if (!socket->waitForConnected(3000)) {
        if (ui->listWidget) {
            addSystemBubble(QStringLiteral("连接聊天服务器失败: ") + socket->errorString());
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
            addSystemBubble(QStringLiteral("好友服务不可用，已加载演示联系人"));
        }
        return;
    }

    ui->chat_user_list->clear();
    currentPeer.clear();

    if (response.isEmpty()) {
        updateCurrentPeer(QString());
        if (ui->listWidget) {
            addSystemBubble(QStringLiteral("当前账号暂无好友"));
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
    chat_user_wid->setSelected(selectByDefault);
    QListWidgetItem *item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->addItem(item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    if (selectByDefault) {
        ui->chat_user_list->setCurrentItem(item);
        chat_user_wid->setSelected(true);
        updateCurrentPeer(chat_user_wid->userName());
    }
}

void Chat_Dialog::addMessageBubble(const QString &sender, const QString &text, bool outgoing)
{
    auto *container = new QWidget;
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(0);

    auto *bubble = new ChatBubble(outgoing ? ChatBubble::Outgoing : ChatBubble::Incoming, sender, text);
    bubble->setMaximumWidth(420);
    bubble->setAttribute(Qt::WA_StyledBackground, true);

    if (outgoing) {
        layout->addStretch();
        layout->addWidget(bubble);
    } else {
        layout->addWidget(bubble);
        layout->addStretch();
    }

    auto *item = new QListWidgetItem(ui->listWidget);
    container->adjustSize();
    item->setSizeHint(container->sizeHint());
    ui->listWidget->addItem(item);
    ui->listWidget->setItemWidget(item, container);
    animateListItem(container);
    ui->listWidget->scrollToBottom();
}

void Chat_Dialog::addSystemBubble(const QString &text)
{
    auto *container = new QWidget;
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(0);

    auto *bubble = new ChatBubble(ChatBubble::System, QString(), text);
    bubble->setMaximumWidth(480);
    layout->addStretch();
    layout->addWidget(bubble);
    layout->addStretch();

    auto *item = new QListWidgetItem(ui->listWidget);
    container->adjustSize();
    item->setSizeHint(container->sizeHint());
    ui->listWidget->addItem(item);
    ui->listWidget->setItemWidget(item, container);
    animateListItem(container);
    ui->listWidget->scrollToBottom();
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
    for (int i = 0; i < ui->chat_user_list->count(); ++i) {
        auto *iterItem = ui->chat_user_list->item(i);
        if (auto *wid = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(iterItem))) {
            wid->setSelected(iterItem == item);
        }
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
           addMessageBubble(QString(), Send_data, true);
           ui->chat_edit->clear();
           Send_data.clear();
       }
}

void Chat_Dialog::on_readyRead()
{
    m_recvBuf.append(socket->readAll());
    processPendingData();
}

void Chat_Dialog::processPendingData()
{
    while (true) {
        if (m_recvFile) {
            if (!consumeFileBytes()) break;
            continue;
        }

        int nl = m_recvBuf.indexOf('\n');
        if (nl < 0) break;

        QByteArray lineData = m_recvBuf.left(nl);
        m_recvBuf.remove(0, nl + 1);
        QString message = QString::fromUtf8(lineData).trimmed();

        if (message.isEmpty()) continue;

        if (message.startsWith(QStringLiteral("MESSAGE::"))) {
            const QStringList parts = message.split(QStringLiteral("::"));
            if (parts.size() >= 3) {
                const QString sender = parts.value(1);
                const QString content = parts.mid(2).join(QStringLiteral("::"));
                addMessageBubble(sender, content, false);
            }
        } else if (message.startsWith(QStringLiteral("FILE::"))) {
            const QStringList parts = message.split(QStringLiteral("::"));
            if (parts.size() >= 5) {
                m_inFileName = parts[1];
                m_fileExp = parts[2].toLongLong();
                m_inFileSender = parts[3];

                m_fileGot = 0;
                m_recvFile = m_fileExp > 0;
                m_targetSavePath.clear();

                // 微信模式：后台自动下载到临时文件夹
                QString tempPath = QDir::tempPath() + "/WeiChat_recv_" + m_inFileName;
                m_inFile.setFileName(tempPath);
                if (!m_inFile.open(QIODevice::WriteOnly)) {
                     // 如果临时文件打不开，可以用一个 uuid 命名重试
                     tempPath = QDir::tempPath() + "/" + QUuid::createUuid().toString() + "_" + m_inFileName;
                     m_inFile.setFileName(tempPath);
                     m_inFile.open(QIODevice::WriteOnly);
                }

                // 创建气泡控件
                m_currentRecvFileWid = new ChatFileWid(ChatFileWid::Receiver, m_inFileName, m_fileExp, m_inFileSender);
                QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
                item->setSizeHint(m_currentRecvFileWid->sizeHint());
                ui->listWidget->addItem(item);
                ui->listWidget->setItemWidget(item, m_currentRecvFileWid);
                animateListItem(m_currentRecvFileWid);
                
                connect(m_currentRecvFileWid, &ChatFileWid::sig_downloadClicked, this, &Chat_Dialog::onDownloadClicked);

                if (!m_recvBuf.isEmpty()) consumeFileBytes();
            }
        } else {
            addSystemBubble(message);
        }
    }
}

bool Chat_Dialog::consumeFileBytes()
{
    if (!m_recvFile || !m_inFile.isOpen()) return false;
    
    qint64 remaining = m_fileExp - m_fileGot;
    if (remaining <= 0) {
        resetIncomingFileState();
        return true;
    }

    qint64 chunkSize = qMin<qint64>(m_recvBuf.size(), remaining);
    if (chunkSize <= 0) return false;

    QByteArray chunk = m_recvBuf.left(static_cast<int>(chunkSize));
    m_recvBuf.remove(0, static_cast<int>(chunkSize));
    qint64 written = m_inFile.write(chunk);
    if (written != chunk.size()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("写入文件失败"));
        resetIncomingFileState();
        return false;
    }

    m_fileGot += written;
    if (m_currentRecvFileWid) {
        m_currentRecvFileWid->updateProgress(m_fileGot, m_fileExp);
    }

    if (m_fileGot >= m_fileExp) {
        if (m_currentRecvFileWid) m_currentRecvFileWid->setCompleted();
        
        // 如果用户在下载中已经选好了路径，收完立即移动/拷贝过去
        if (!m_targetSavePath.isEmpty()) {
            m_inFile.close();
            QFile::remove(m_targetSavePath); // 先删除已存在的
            if (QFile::copy(m_inFile.fileName(), m_targetSavePath)) {
                QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("文件已保存至：") + m_targetSavePath);
            }
        }
        resetIncomingFileState();
        return true;
    }

    return !m_recvBuf.isEmpty();
}

void Chat_Dialog::resetIncomingFileState()
{
    if (m_inFile.isOpen()) m_inFile.close();
    m_recvFile = false;
    m_fileExp = 0;
    m_fileGot = 0;
    m_inFileName.clear();
    m_inFileSender.clear();
    m_currentRecvFileWid = nullptr;
}

void Chat_Dialog::animateListItem(QWidget *widget)
{
    if (!widget) return;
    QTimer::singleShot(0, widget, [widget]() {
        auto *effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
        effect->setOpacity(0.0);

        const QPoint endPos = widget->pos();
        const QPoint startPos = endPos + QPoint(0, 12);
        widget->move(startPos);

        auto *opacityAnim = new QPropertyAnimation(effect, "opacity", widget);
        opacityAnim->setDuration(220);
        opacityAnim->setStartValue(0.0);
        opacityAnim->setEndValue(1.0);
        opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

        auto *moveAnim = new QPropertyAnimation(widget, "pos", widget);
        moveAnim->setDuration(220);
        moveAnim->setStartValue(startPos);
        moveAnim->setEndValue(endPos);
        moveAnim->setEasingCurve(QEasingCurve::OutCubic);

        opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
        moveAnim->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

bool Chat_Dialog::eventFilter(QObject *watched, QEvent *event)
{
    return QDialog::eventFilter(watched, event);
}

void Chat_Dialog::onFileClicked()
{
    if (currentPeer.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先在左侧选择聊天对象"));
        return;
    }
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("未连接到服务器"));
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择要发送的文件"));
    if (filePath.isEmpty()) return;

    if (m_sendFile.isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("有文件正在发送中，请稍后再试"));
        return;
    }

    QFileInfo fi(filePath);
    m_sendFilePath = filePath;
    m_sendFileSize = fi.size();
    m_sendFileSent = 0;

    m_sendFile.setFileName(filePath);
    if (!m_sendFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法读取文件"));
        return;
    }

    QString header = "FILE::" + fi.fileName() + "::" + QString::number(m_sendFileSize) +
                     "::" + username + "::" + currentPeer + "\n";
    socket->write(header.toUtf8());

    // 创建发送端气泡
    m_currentSendFileWid = new ChatFileWid(ChatFileWid::Sender, fi.fileName(), m_sendFileSize, currentPeer);
    QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
    item->setSizeHint(m_currentSendFileWid->sizeHint());
    ui->listWidget->addItem(item);
    ui->listWidget->setItemWidget(item, m_currentSendFileWid);
    animateListItem(m_currentSendFileWid);

    sendNextChunk();
}

void Chat_Dialog::sendNextChunk()
{
    if (!m_sendFile.isOpen() || !socket) return;
    QByteArray chunk = m_sendFile.read(SEND_CHUNK);
    if (!chunk.isEmpty()) socket->write(chunk);
}

void Chat_Dialog::onBytesSent(qint64 bytes)
{
    if (!m_sendFile.isOpen()) return;

    m_sendFileSent += bytes;
    if (m_currentSendFileWid) {
        m_currentSendFileWid->updateProgress(m_sendFileSent, m_sendFileSize);
    }

    if (m_sendFileSent >= m_sendFileSize || m_sendFile.atEnd()) {
        m_sendFile.close();
        if (m_currentSendFileWid) m_currentSendFileWid->setCompleted();
        m_currentSendFileWid = nullptr;
        m_sendFilePath.clear();
        m_sendFileSize = 0;
        m_sendFileSent = 0;
        return;
    }

    if (socket->bytesToWrite() < SEND_CHUNK * 2) {
        sendNextChunk();
    }
}

void Chat_Dialog::onDownloadClicked(const QString &fileName, qint64 fileSize, const QString &peerName)
{
    Q_UNUSED(fileSize);
    Q_UNUSED(peerName);

    QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("另存为"), fileName);
    if (savePath.isEmpty()) return;

    m_targetSavePath = savePath;

    // 如果文件已经接收全了（缓存在临时目录中）
    if (!m_recvFile && QFile::exists(m_inFile.fileName())) {
        if (QFile::copy(m_inFile.fileName(), m_targetSavePath)) {
             QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("文件已保存至：") + m_targetSavePath);
        } else {
             QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("保存失败，可能文件已被移动或权限不足"));
        }
    } else if (m_recvFile) {
        // 如果还在下载中，只需设置 m_targetSavePath，在 consumeFileBytes 结束时会自动拷贝
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("正在后台下载，完成后将自动保存到：") + m_targetSavePath);
    }
}

void Chat_Dialog::on_disconnected()
{
    qDebug() << "Disconnected from server!";
    addSystemBubble(QStringLiteral("与服务器断开连接"));
}
