#include "chat_dialog.h"
#include "ui_chat_dialog.h"
#include "animatediconbutton.h"
#include "chatmessagedelegate.h"
#include "chatmessagelistmodel.h"
#include<QRandomGenerator>

#include<QAction>
#include<QAbstractItemView>
#include <QBuffer>
#include<QCloseEvent>
#include<QResizeEvent>
#include <QDesktopServices>
#include<QIcon>
#include<QHBoxLayout>
#include<QLineEdit>
#include<QListWidgetItem>
#include<QListView>
#include<QMessageBox>
#include<QPushButton>
#include<QLabel>
#include<QLayout>
#include<QTextEdit>
#include<QTcpSocket>
#include<QStringList>
#include<QGraphicsDropShadowEffect>
#include<QPropertyAnimation>
#include<QTimer>
#include<vector>
#include<chatuserwid.h>
#include<QFileDialog>
#include<QFileInfo>
#include <QFutureWatcher>
#include<QDir>
#include <QImageReader>
#include<QMouseEvent>
#include <QtConcurrent>
#include<QUuid>
#include<QStyle>
#include<QFrame>
#include<QPainter>
#include<QSvgRenderer>
#include<QScrollBar>
#include <QSet>
#include<QTextCursor>
#include <QUrl>

namespace {

struct ButtonThemeColors {
    QColor base;
    QColor hover;
    QColor press;
    QColor icon;
    QString themeIconPath;
};

ButtonThemeColors buttonThemeColors(const QString &theme)
{
    const bool light = (theme == QStringLiteral("light"));
    return {
        light ? QColor(30, 31, 33, 220) : QColor(220, 223, 228, 220),
        light ? QColor(10, 10, 12, 255) : QColor(255, 255, 255, 230),
        light ? QColor(0, 0, 0, 200) : QColor(255, 255, 255, 200),
        light ? QColor(28, 30, 34) : QColor(228, 231, 236),
        light ? QStringLiteral(":/svg/sun.svg") : QStringLiteral(":/svg/moon.svg")
    };
}

QPixmap renderSvgPixmap(const QString &path, const QSize &size, const QColor &tint)
{
    QSvgRenderer renderer(path);
    if (!renderer.isValid()) {
        return QPixmap();
    }

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), tint);
    painter.end();
    return QPixmap::fromImage(image);
}

QPixmap tintPixmap(const QString &path, const QSize &size, const QColor &tint)
{
    QPixmap source(path);
    if (source.isNull()) {
        return QPixmap();
    }

    QPixmap scaled = size.isValid()
        ? source.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        : source;
    QImage image = scaled.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), tint);
    painter.end();

    return QPixmap::fromImage(image);
}

bool isImagePath(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    static const QSet<QString> formats = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("webp")
    };
    return formats.contains(suffix);
}

QByteArray createThumbnailData(const QString &path, const QSize &targetSize = QSize(200, 200))
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        return QByteArray();
    }

    const QImage thumb = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QByteArray();
    }
    if (!thumb.save(&buffer, "JPEG", 80)) {
        return QByteArray();
    }
    return data;
}

void repolishRecursively(QWidget *root)
{
    if (!root) {
        return;
    }

    QList<QWidget *> widgets = root->findChildren<QWidget *>();
    widgets.prepend(root);
    for (QWidget *widget : widgets) {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
}

QString normalizeHostForConnect(QString host)
{
    host = host.trimmed();
    if (host.startsWith('[') && host.endsWith(']') && host.size() > 2) {
        host = host.mid(1, host.size() - 2);
    }
    return host;
}

}
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
    m_searchAction = new QAction(ui->search_edit);
    ui->search_edit->addAction(m_searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));
    // 创建一个清除动作并设置图标
    m_clearAction = new QAction(ui->search_edit);
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(m_clearAction, QLineEdit::TrailingPosition);
    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [this](const QString &) {
        updateActionIcons();
    });
    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(m_clearAction, &QAction::triggered, [this]() {
        ui->search_edit->clear();
        updateActionIcons();
        ui->search_edit->clearFocus();
        //清除按钮被按下则不显示搜索框
        //ShowSearch(false);
    });
    ui->search_edit->SetMaxLength(15);
    ui->chat_user_list->setSpacing(6);
    setupModernMessageView();
    initializeStorage();

    if (ui->verticalLayout_4) {
        ui->verticalLayout_4->setStretch(0, 1);
        ui->verticalLayout_4->setStretch(1, 0);
        ui->verticalLayout_4->setStretch(2, 0);
        ui->verticalLayout_4->setStretch(3, 0);
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

    m_friendRefreshTimer = new QTimer(this);
    m_friendRefreshTimer->setInterval(5000);
    connect(m_friendRefreshTimer, &QTimer::timeout, this, &Chat_Dialog::loadFriendList);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(5000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &Chat_Dialog::sendHeartbeat);

    const QList<QLabel *> clickableLabels = {
        ui->side_connect_lb, ui->side_head_lb, ui->label_2, ui->label_3, ui->side_chat_lb,
        ui->label_4, ui->label_6, ui->label_5, ui->emo_lb, ui->label_8, ui->label_9, ui->label_7
    };
    for (QLabel *label : clickableLabels) {
        if (!label) {
            continue;
        }
        label->setCursor(Qt::PointingHandCursor);
        label->installEventFilter(this);
    }

    addChatUSerList();
    initializeConnection();

    applyTheme(qApp->property("theme").toString().isEmpty() ? QStringLiteral("dark") : qApp->property("theme").toString());

    if (m_fileButton) {
        connect(m_fileButton, &QPushButton::clicked, this, &Chat_Dialog::onFileClicked);
    }
}

Chat_Dialog::~Chat_Dialog()
{
    if (!currentPeer.isEmpty() && ui->chat_edit) {
        const QString draft = ui->chat_edit->toPlainText();
        m_draftCache.insert(currentPeer, draft);
        m_storage.saveDraft(username, currentPeer, draft);
    }

    qDeleteAll(m_conversationModels);
    m_conversationModels.clear();

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

void Chat_Dialog::setupModernMessageView()
{
    if (!ui->message_view) {
        return;
    }

    m_messageView = ui->message_view;
    m_messageView->setObjectName(QStringLiteral("modern_message_view"));
    m_messageView->setFrameShape(QFrame::NoFrame);
    m_messageView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_messageView->setSelectionMode(QAbstractItemView::NoSelection);
    m_messageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_messageView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_messageView->setSpacing(6);
    m_messageView->setWordWrap(true);
    m_messageView->setUniformItemSizes(false);

    m_emptyMessageModel = new ChatMessageListModel(this);
    m_messageModel = m_emptyMessageModel;
    m_messageDelegate = new ChatMessageDelegate(this);
    m_messageView->setModel(m_messageModel);
    m_messageView->setItemDelegate(m_messageDelegate);
    connect(m_messageDelegate, &ChatMessageDelegate::downloadRequested, this, &Chat_Dialog::onDownloadClicked);
    connect(m_messageDelegate, &ChatMessageDelegate::imageRequested, this, &Chat_Dialog::onImageClicked);

    m_messageView->viewport()->installEventFilter(this);
    connect(m_messageView->verticalScrollBar(), &QScrollBar::rangeChanged, this,
            [this](int, int) { QTimer::singleShot(0, this, [this]() { refreshMessageListLayout(); }); });
}

void Chat_Dialog::initializeStorage()
{
    if (!m_storage.initialize(username)) {
        addSystemBubble(QStringLiteral("本地消息存储初始化失败，当前仅临时会话可用"));
        return;
    }
    m_dbPath = m_storage.databasePath();
}

ChatMessageListModel *Chat_Dialog::loadConversationModel(const QString &peerName)
{
    if (peerName.isEmpty()) {
        return m_emptyMessageModel;
    }

    auto it = m_conversationModels.find(peerName);
    if (it != m_conversationModels.end()) {
        return it.value();
    }

    auto *model = new ChatMessageListModel(this);
    const auto messages = m_storage.loadConversation(username, peerName, 2000);
    for (const auto &msg : messages) {
        model->appendMessage(msg);
    }
    m_conversationModels.insert(peerName, model);
    return model;
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

    const QString host = normalizeHostForConnect(serverConfig.host);
    socket->connectToHost(host, serverConfig.chatPort);
    if (!socket->waitForConnected(3000)) {
        addSystemBubble(QStringLiteral("连接聊天服务器失败: ") + socket->errorString());
        return;
    }

    const QString onlineMessage = QStringLiteral("ONLINE::") + username + QStringLiteral("\n");
    socket->write(onlineMessage.toUtf8());
    socket->flush();

    loadFriendList();
    if (m_friendRefreshTimer && !m_friendRefreshTimer->isActive()) {
        m_friendRefreshTimer->start();
    }
    if (m_heartbeatTimer && !m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start();
    }
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
        const bool online = (randomValue % 2) == 0;
        addChatUserItem(names[name_i], heads[head_i], strs[str_i], online, i == 0);
    }
}

void Chat_Dialog::loadFriendList()
{
    const QString selectedPeer = currentPeer;
    const QString response = sendFriendCommand(QStringLiteral("NOWMYFRIEND::") + username);
    if (response == QStringLiteral("__FRIEND_SERVER_ERROR__")) {
        addChatUSerList();
        addSystemBubble(QStringLiteral("好友服务不可用，已加载演示联系人"));
        return;
    }

    ui->chat_user_list->clear();
    bool selectedFound = false;

    if (response.isEmpty()) {
        updateCurrentPeer(QString());
        addSystemBubble(QStringLiteral("当前账号暂无好友"));
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

        const bool online = fields.value(1).trimmed() == QStringLiteral("1");
        const QString preview = online ? QStringLiteral("当前在线") : QStringLiteral("当前离线");
        const QString head = heads.at(index % static_cast<int>(heads.size()));
        const bool selectByDefault = selectedPeer.isEmpty() ? (index == 0) : (friendName == selectedPeer);
        if (selectByDefault) {
            selectedFound = true;
        }
        addChatUserItem(friendName, head, preview, online, selectByDefault);
        ++index;
    }

    if (index == 0) {
        addChatUSerList();
    } else if (!selectedPeer.isEmpty() && !selectedFound) {
        updateCurrentPeer(QString());
    }
}

QString Chat_Dialog::sendFriendCommand(const QString &command)
{
    QTcpSocket friendSocket;
    friendSocket.connectToHost(normalizeHostForConnect(serverConfig.host), serverConfig.friendPort);
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
    if (m_heartbeatTimer && m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();
    }
    QTcpSocket offlineSocket;
    offlineSocket.connectToHost(normalizeHostForConnect(serverConfig.host), serverConfig.offlinePort);
    if (!offlineSocket.waitForConnected(1000)) {
        return;
    }

    offlineSocket.write(username.toUtf8());
    offlineSocket.waitForBytesWritten(1000);
    offlineSocket.disconnectFromHost();
}

void Chat_Dialog::sendHeartbeat()
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState || username.isEmpty()) {
        return;
    }
    const QString heartbeat = QStringLiteral("HEARTBEAT::") + username + QStringLiteral("\n");
    socket->write(heartbeat.toUtf8());
}

void Chat_Dialog::addChatUserItem(const QString &name, const QString &head, const QString &preview, bool online, bool selectByDefault)
{
    auto *chat_user_wid = new ChatUserWid();
    chat_user_wid->SetInfo(name, head, preview, online);
    if (!name.isEmpty() && !head.isEmpty()) {
        m_peerAvatar.insert(name, head);
    }
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

void Chat_Dialog::applyTheme(const QString &theme)
{
    m_theme = theme;
    setProperty("theme", theme);

    const ButtonThemeColors colors = buttonThemeColors(theme);
    for (auto *btn : findChildren<AnimatedIconButton*>()) {
        btn->setBaseColor(colors.base);
        btn->setHoverColor(colors.hover);
        btn->setPressColor(colors.press);
    }

    updateActionIcons();
    updateDecorativeIcons();
    if (m_messageDelegate) {
        m_messageDelegate->setTheme(theme);
    }
    if (m_messageView) {
        m_messageView->viewport()->update();
    }
    repolishRecursively(this);
}

void Chat_Dialog::updateActionIcons()
{
    const ButtonThemeColors colors = buttonThemeColors(m_theme);
    const QSize iconSize(16, 16);

    if (m_searchAction) {
        m_searchAction->setIcon(QIcon(renderSvgPixmap(QStringLiteral(":/svg/search.svg"), iconSize, colors.icon)));
    }
    if (m_clearAction) {
        m_clearAction->setIcon(QIcon(renderSvgPixmap(QStringLiteral(":/svg/close.svg"), iconSize, colors.icon)));
    }
}

void Chat_Dialog::updateDecorativeIcons()
{
    const ButtonThemeColors colors = buttonThemeColors(m_theme);

    if (ui->side_head_lb) {
        ui->side_head_lb->setPixmap(tintPixmap(QStringLiteral(":/png/left_1.png"), QSize(24, 24), colors.icon));
    }
    if (ui->label_2) {
        ui->label_2->setPixmap(tintPixmap(QStringLiteral(":/png/user.png"), QSize(24, 24), colors.icon));
    }
    if (ui->label_3) {
        ui->label_3->setPixmap(tintPixmap(QStringLiteral(":/png/left_3.png"), QSize(24, 24), colors.icon));
    }
    if (ui->side_chat_lb) {
        ui->side_chat_lb->setPixmap(tintPixmap(QStringLiteral(":/png/friend.png"), QSize(24, 24), colors.icon));
    }
    if (ui->label_7) {
        ui->label_7->setPixmap(renderSvgPixmap(QStringLiteral(":/svg/more.svg"), QSize(18, 18), colors.icon));
    }
    if (ui->emo_lb) {
        ui->emo_lb->setPixmap(renderSvgPixmap(QStringLiteral(":/svg/emoji.svg"), QSize(18, 18), colors.icon));
    }
    if (ui->label_8) {
        ui->label_8->setPixmap(tintPixmap(QStringLiteral(":/png/jietu.png"), QSize(18, 18), colors.icon));
    }
    if (ui->label_9) {
        ui->label_9->setPixmap(tintPixmap(QStringLiteral(":/png/record.png"), QSize(18, 18), colors.icon));
    }
}

void Chat_Dialog::addMessageBubble(const QString &sender, const QString &text, bool outgoing, const QDateTime &timestamp)
{
    if (!m_messageModel || !m_messageView) {
        return;
    }

    ChatMessageListModel::MessageItem item;
    item.type = ChatMessageListModel::MessageItem::Text;
    item.sender = sender;
    item.text = text;
    item.outgoing = outgoing;
    item.system = false;
    item.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    m_messageModel->appendMessage(item);
    scrollMessagesAnimated();
}

void Chat_Dialog::addSystemBubble(const QString &text, const QDateTime &timestamp)
{
    if (!m_messageModel || !m_messageView) {
        return;
    }

    ChatMessageListModel::MessageItem item;
    item.type = ChatMessageListModel::MessageItem::System;
    item.sender.clear();
    item.text = text;
    item.outgoing = false;
    item.system = true;
    item.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    m_messageModel->appendMessage(item);
    scrollMessagesAnimated();
}

void Chat_Dialog::addFileBubble(const QString &sender, const QString &fileName, qint64 fileSize, qint64 progress,
                                bool outgoing, bool completed, bool downloadable, const QDateTime &timestamp)
{
    if (!m_messageModel || !m_messageView) {
        return;
    }

    ChatMessageListModel::MessageItem item;
    item.type = ChatMessageListModel::MessageItem::File;
    item.sender = sender;
    item.text = QStringLiteral("文件消息");
    item.outgoing = outgoing;
    item.system = false;
    item.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    item.fileName = fileName;
    item.fileSize = fileSize;
    item.fileProgress = progress;
    item.fileCompleted = completed;
    item.downloadable = downloadable;
    m_messageModel->appendMessage(item);
    scrollMessagesAnimated();
}

qint64 Chat_Dialog::appendPeerMessage(const QString &peerName, const QString &sender, const QString &text,
                                      bool outgoing, bool system, const QDateTime &timestamp)
{
    if (peerName.isEmpty()) {
        return -1;
    }

    ChatMessageListModel::MessageItem item;
    item.type = system ? ChatMessageListModel::MessageItem::System : ChatMessageListModel::MessageItem::Text;
    item.sender = sender;
    item.text = text;
    item.outgoing = outgoing;
    item.system = system;
    item.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    item.messageId = m_storage.addMessage(username, peerName, item);

    ChatMessageListModel *model = loadConversationModel(peerName);
    if (model) {
        model->appendMessage(item);
    }
    return item.messageId;
}

void Chat_Dialog::sendOfflineAck(qint64 offlineId)
{
    if (offlineId <= 0 || !socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    const QString ack = QStringLiteral("ACK::%1\n").arg(offlineId);
    socket->write(ack.toUtf8());
}

qint64 Chat_Dialog::appendPeerFileMessage(const QString &peerName, const QString &sender, const QString &fileName,
                                          qint64 fileSize, bool outgoing, bool downloadable,
                                          const QString &localPath, bool isImage, const QByteArray &thumbnailData)
{
    if (peerName.isEmpty()) {
        return -1;
    }

    ChatMessageListModel::MessageItem item;
    item.type = ChatMessageListModel::MessageItem::File;
    item.sender = sender;
    item.outgoing = outgoing;
    item.system = false;
    item.timestamp = QDateTime::currentDateTime();
    item.fileName = fileName;
    item.fileSize = fileSize;
    item.fileProgress = 0;
    item.fileCompleted = false;
    item.downloadable = downloadable;
    item.localPath = localPath;
    item.isImage = isImage;
    item.thumbnailData = thumbnailData;
    if (!thumbnailData.isEmpty()) {
        item.thumbnailPixmap.loadFromData(thumbnailData);
    }
    item.messageId = m_storage.addMessage(username, peerName, item);

    ChatMessageListModel *model = loadConversationModel(peerName);
    if (model) {
        model->appendMessage(item);
    }
    return item.messageId;
}

void Chat_Dialog::updatePeerFileProgress(const QString &peerName, qint64 messageId, qint64 progress, bool completed,
                                         bool downloadable, const QString &localPath)
{
    if (peerName.isEmpty() || messageId < 0) {
        return;
    }

    ChatMessageListModel *model = loadConversationModel(peerName);
    if (model) {
        const int row = model->findRowByMessageId(messageId);
        if (row >= 0) {
            auto item = model->messageAt(row);
            item.fileProgress = progress;
            item.fileCompleted = completed;
            item.downloadable = downloadable;
            if (!localPath.isEmpty()) {
                item.localPath = localPath;
            }
            model->updateMessage(row, item);
        }
    }

    m_storage.updateFileProgress(username, peerName, messageId, progress, completed, downloadable, localPath);
}

void Chat_Dialog::updatePeerImageData(const QString &peerName, qint64 messageId, bool isImage,
                                      const QByteArray &thumbnailData, const QString &localPath)
{
    if (peerName.isEmpty() || messageId < 0) {
        return;
    }

    ChatMessageListModel *model = loadConversationModel(peerName);
    if (model) {
        const int row = model->findRowByMessageId(messageId);
        if (row >= 0) {
            auto item = model->messageAt(row);
            item.isImage = isImage;
            item.thumbnailData = thumbnailData;
            if (!thumbnailData.isEmpty()) {
                item.thumbnailPixmap.loadFromData(thumbnailData);
            }
            if (!localPath.isEmpty()) {
                item.localPath = localPath;
            }
            model->updateMessage(row, item);
        }
    }

    m_storage.updateImageData(username, peerName, messageId, isImage, thumbnailData, localPath);
}

void Chat_Dialog::requestThumbnailGeneration(const QString &peerName, qint64 messageId, const QString &localPath)
{
    if (peerName.isEmpty() || messageId < 0 || localPath.isEmpty() || !QFile::exists(localPath)) {
        return;
    }

    auto *watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [this, watcher, peerName, messageId, localPath]() {
        const QByteArray thumb = watcher->result();
        watcher->deleteLater();
        if (thumb.isEmpty()) {
            return;
        }
        updatePeerImageData(peerName, messageId, true, thumb, localPath);
    });

    watcher->setFuture(QtConcurrent::run([localPath]() {
        return createThumbnailData(localPath);
    }));
}

void Chat_Dialog::renderConversation(const QString &peerName)
{
    if (!m_messageView) {
        return;
    }
    if (peerName.isEmpty()) {
        m_messageModel = m_emptyMessageModel;
        m_messageView->setModel(m_messageModel);
        m_messageView->setItemDelegate(m_messageDelegate);
        return;
    }

    ChatMessageListModel *model = loadConversationModel(peerName);
    if (!model) {
        return;
    }

    m_messageModel = model;
    m_messageView->setModel(m_messageModel);
    m_messageView->setItemDelegate(m_messageDelegate);
    m_messageView->scrollToBottom();
}

void Chat_Dialog::clearCurrentConversation()
{
    if (currentPeer.isEmpty()) {
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("清空历史"),
        QStringLiteral("确定清空与 %1 的历史消息吗？此操作不可撤销。").arg(currentPeer),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!m_storage.clearConversation(username, currentPeer)) {
        QMessageBox::warning(this, QStringLiteral("清空历史"), QStringLiteral("清空失败，请稍后重试"));
        return;
    }
    if (m_conversationModels.contains(currentPeer)) {
        m_conversationModels[currentPeer]->clear();
    }
    m_draftCache.remove(currentPeer);
    if (ui->chat_edit) {
        ui->chat_edit->clear();
    }
}

void Chat_Dialog::updateCurrentPeer(const QString &peerName)
{
    const QString oldPeer = currentPeer;
    if (!oldPeer.isEmpty() && ui->chat_edit) {
        const QString oldDraft = ui->chat_edit->toPlainText();
        m_draftCache.insert(oldPeer, oldDraft);
        m_storage.saveDraft(username, oldPeer, oldDraft);
    }

    const bool peerChanged = (currentPeer != peerName);
    currentPeer = peerName;
    if (ui->label) {
        ui->label->setText(peerName.isEmpty() ? QStringLiteral("请选择聊天对象") : peerName);
    }
    if (peerChanged) {
        renderConversation(peerName);

        if (ui->chat_edit) {
            QString draft = m_draftCache.value(peerName);
            if (draft.isNull() && !peerName.isEmpty()) {
                draft = m_storage.loadDraft(username, peerName);
                m_draftCache.insert(peerName, draft);
            }
            ui->chat_edit->blockSignals(true);
            ui->chat_edit->setPlainText(draft);
            ui->chat_edit->moveCursor(QTextCursor::End);
            ui->chat_edit->blockSignals(false);
            Send_data = draft;
        }
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
    Send_data = currentText;

    if (!currentPeer.isEmpty()) {
        m_draftCache.insert(currentPeer, currentText);
        m_storage.saveDraft(username, currentPeer, currentText);
    }
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
             appendPeerMessage(currentPeer, username, Send_data, true, false);
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
                appendPeerMessage(sender, sender, content, false, false);
            }
        } else if (message.startsWith(QStringLiteral("OFFLINE::"))) {
            const QStringList parts = message.split(QStringLiteral("::"));
            if (parts.size() >= 5) {
                bool idOk = false;
                const qint64 offlineId = parts.value(1).toLongLong(&idOk);
                const QString sender = parts.value(2);
                const QByteArray decoded = QByteArray::fromBase64(parts.value(3).toLatin1());
                const QString content = QString::fromUtf8(decoded);
                const QDateTime ts = QDateTime::fromString(parts.value(4), QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));

                if (!sender.isEmpty()) {
                    appendPeerMessage(sender, sender, content, false, false, ts);
                    if (idOk) {
                        sendOfflineAck(offlineId);
                    }
                }
            }
        } else if (message.startsWith(QStringLiteral("FILE::"))) {
            const QStringList parts = message.split(QStringLiteral("::"));
            if (parts.size() >= 5) {
                m_inFileName = parts[1];
                m_fileExp = parts[2].toLongLong();
                m_inFileSender = parts[3];
                m_incomingPeer = m_inFileSender;
                const bool incomingIsImage = isImagePath(m_inFileName);

                m_currentRecvMessageId = appendPeerFileMessage(m_incomingPeer, m_inFileSender, m_inFileName, m_fileExp,
                                                                false, true, QString(), incomingIsImage);

                PendingIncomingFile pending;
                pending.sender = m_inFileSender;
                pending.receiver = parts[4];
                pending.fileName = m_inFileName;
                pending.fileSize = m_fileExp;
                m_pendingIncomingFiles.insert(m_currentRecvMessageId, pending);

                if (m_incomingPeer == currentPeer) {
                    addSystemBubble(QStringLiteral("收到文件：%1，点击下载开始接收").arg(m_inFileName));
                }
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
    updatePeerFileProgress(m_incomingPeer, m_currentRecvMessageId, m_fileGot, false, true, m_inFile.fileName());

    if (m_fileGot >= m_fileExp) {
        updatePeerFileProgress(m_incomingPeer, m_currentRecvMessageId, m_fileExp, true, true, m_inFile.fileName());
        if (isImagePath(m_inFileName)) {
            requestThumbnailGeneration(m_incomingPeer, m_currentRecvMessageId, m_inFile.fileName());
        }
        if (m_incomingPeer == currentPeer) {
            addSystemBubble(QStringLiteral("文件接收完成：%1").arg(m_inFileName));
        }
        
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
    m_incomingPeer.clear();
    m_targetSavePath.clear();
    m_currentRecvMessageId = -1;
}

void Chat_Dialog::animateListItem()
{
    scrollMessagesAnimated();
}

void Chat_Dialog::scrollMessagesAnimated()
{
    if (!m_messageView || !m_messageView->verticalScrollBar()) {
        return;
    }

    QScrollBar *bar = m_messageView->verticalScrollBar();
    const int endValue = bar->maximum();
    const int startValue = qMax(0, endValue - 42);

    auto *animation = new QPropertyAnimation(bar, "value", this);
    animation->setDuration(180);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Chat_Dialog::refreshMessageListLayout()
{
    if (!m_messageView) {
        return;
    }
    m_messageView->doItemsLayout();
    m_messageView->viewport()->update();
}

void Chat_Dialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    refreshMessageListLayout();
}

bool Chat_Dialog::eventFilter(QObject *watched, QEvent *event)
{
    if (m_messageView && watched == m_messageView->viewport()) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::LayoutRequest || event->type() == QEvent::Show) {
            QTimer::singleShot(0, this, [this]() { refreshMessageListLayout(); });
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == ui->side_head_lb) {
            showFeatureComingSoon(QStringLiteral("聊天列表"));
            return true;
        }
        if (watched == ui->label_2) {
            showFeatureComingSoon(QStringLiteral("通讯录"));
            return true;
        }
        if (watched == ui->label_3) {
            showFeatureComingSoon(QStringLiteral("发现"));
            return true;
        }
        if (watched == ui->side_chat_lb) {
            loadFriendList();
            QMessageBox::information(this, QStringLiteral("好友"), QStringLiteral("已刷新好友列表"));
            return true;
        }
        if (watched == ui->label_4) {
            showFeatureComingSoon(QStringLiteral("设置"));
            return true;
        }
        if (watched == ui->label_6) {
            showFeatureComingSoon(QStringLiteral("通话"));
            return true;
        }
        if (watched == ui->label_5) {
            showFeatureComingSoon(QStringLiteral("更多"));
            return true;
        }
        if (watched == ui->side_connect_lb) {
            showFeatureComingSoon(QStringLiteral("个人资料"));
            return true;
        }
        if (watched == ui->emo_lb) {
            showFeatureComingSoon(QStringLiteral("表情"));
            return true;
        }
        if (watched == ui->label_8) {
            showFeatureComingSoon(QStringLiteral("截图"));
            return true;
        }
        if (watched == ui->label_9) {
            showFeatureComingSoon(QStringLiteral("语音录制"));
            return true;
        }
        if (watched == ui->label_7) {
            clearCurrentConversation();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void Chat_Dialog::showFeatureComingSoon(const QString &featureName)
{
    QMessageBox::information(this, QStringLiteral("提示"), featureName + QStringLiteral("功能开发中"));
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
    const QByteArray headerBytes = header.toUtf8();
    m_sendHeaderBytesRemaining = headerBytes.size();
    socket->write(headerBytes);

    m_currentSendPeer = currentPeer;
    const bool outgoingIsImage = isImagePath(filePath);
    m_currentSendMessageId = appendPeerFileMessage(currentPeer, username, fi.fileName(), m_sendFileSize,
                                                   true, false, filePath, outgoingIsImage);
    if (outgoingIsImage) {
        requestThumbnailGeneration(currentPeer, m_currentSendMessageId, filePath);
    }

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

    qint64 payloadBytes = bytes;
    if (m_sendHeaderBytesRemaining > 0) {
        const qint64 consumed = qMin(m_sendHeaderBytesRemaining, payloadBytes);
        m_sendHeaderBytesRemaining -= consumed;
        payloadBytes -= consumed;
    }
    if (payloadBytes <= 0) {
        return;
    }

    m_sendFileSent += payloadBytes;
    updatePeerFileProgress(m_currentSendPeer, m_currentSendMessageId, m_sendFileSent, false, false);

    if (m_sendFileSent >= m_sendFileSize || m_sendFile.atEnd()) {
        m_sendFile.close();
        updatePeerFileProgress(m_currentSendPeer, m_currentSendMessageId, m_sendFileSize, true, false);
        if (m_currentSendPeer == currentPeer) {
            addSystemBubble(QStringLiteral("文件发送完成"));
        }
        m_currentSendMessageId = -1;
        m_currentSendPeer.clear();
        m_sendFilePath.clear();
        m_sendFileSize = 0;
        m_sendFileSent = 0;
        m_sendHeaderBytesRemaining = 0;
        return;
    }

    if (socket->bytesToWrite() < SEND_CHUNK * 2) {
        sendNextChunk();
    }
}

void Chat_Dialog::onDownloadClicked(int row)
{
    if (currentPeer.isEmpty() || !m_messageModel) {
        return;
    }
    if (row < 0 || row >= m_messageModel->rowCount()) {
        return;
    }
    ChatMessageListModel::MessageItem message = m_messageModel->messageAt(row);
    if (message.type != ChatMessageListModel::MessageItem::File) {
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("另存为"), message.fileName);
    if (savePath.isEmpty()) return;

    if (!message.localPath.isEmpty() && QFile::exists(message.localPath)) {
        QFile::remove(savePath);
        if (QFile::copy(message.localPath, savePath)) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("文件已保存至：") + savePath);
        } else {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("保存失败，可能权限不足或目标文件被占用"));
        }
        return;
    }

    if (m_recvFile) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("当前有文件正在接收，请稍后再试"));
        return;
    }
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("未连接到服务器"));
        return;
    }

    const qint64 messageId = message.messageId;
    if (!m_pendingIncomingFiles.contains(messageId)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("文件信息已失效，请让对方重新发送"));
        return;
    }

    const PendingIncomingFile pending = m_pendingIncomingFiles.value(messageId);
    m_targetSavePath = savePath;
    m_inFileName = pending.fileName;
    m_inFileSender = pending.sender;
    m_incomingPeer = pending.sender;
    m_currentRecvMessageId = messageId;
    m_fileExp = pending.fileSize;
    m_fileGot = 0;
    m_recvFile = m_fileExp > 0;

    QString tempPath = QDir::tempPath() + "/WeiChat_recv_" + m_inFileName;
    m_inFile.setFileName(tempPath);
    if (!m_inFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        tempPath = QDir::tempPath() + "/" + QUuid::createUuid().toString() + "_" + m_inFileName;
        m_inFile.setFileName(tempPath);
        if (!m_inFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法创建临时文件"));
            resetIncomingFileState();
            return;
        }
    }

    updatePeerFileProgress(m_incomingPeer, m_currentRecvMessageId, 0, false, true, m_inFile.fileName());

    const QString ready = QStringLiteral("FILE_READY::%1::%2::%3::%4\n")
                              .arg(pending.sender)
                              .arg(username)
                              .arg(pending.fileName)
                              .arg(pending.fileSize);
    socket->write(ready.toUtf8());
    m_pendingIncomingFiles.remove(messageId);

    if (m_fileExp == 0) {
        updatePeerFileProgress(m_incomingPeer, m_currentRecvMessageId, 0, true, true, m_inFile.fileName());
        m_inFile.close();
        QFile::remove(m_targetSavePath);
        if (QFile::copy(m_inFile.fileName(), m_targetSavePath)) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("文件已保存至：") + m_targetSavePath);
        }
        resetIncomingFileState();
    }
}

void Chat_Dialog::onImageClicked(int row)
{
    if (currentPeer.isEmpty() || !m_messageModel) {
        return;
    }
    if (row < 0 || row >= m_messageModel->rowCount()) {
        return;
    }

    const ChatMessageListModel::MessageItem message = m_messageModel->messageAt(row);
    if (!message.isImage || message.localPath.isEmpty()) {
        return;
    }
    if (!QFile::exists(message.localPath)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("原图文件不存在，可能已被系统清理"));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(message.localPath))) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法打开图片预览"));
    }
}

void Chat_Dialog::on_disconnected()
{
    if (m_heartbeatTimer && m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();
    }
    qDebug() << "Disconnected from server!";
    addSystemBubble(QStringLiteral("与服务器断开连接"));
}
