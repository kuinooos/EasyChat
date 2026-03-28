#ifndef CHAT_DIALOG_H
#define CHAT_DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include "server_config.h"
#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QVector>
#include <QSize>
#include "chatmessagelistmodel.h"
#include "chatstorage.h"

class QPushButton;
class QListWidgetItem;
class QListView;
class QCloseEvent;
class QResizeEvent;
class AnimatedIconButton;
class QAction;
class QTimer;
class ChatMessageDelegate;

namespace Ui {
class Chat_Dialog;
}

class Chat_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Chat_Dialog(const QString &username, const ServerConfig &serverConfig, QWidget *parent = nullptr);
    ~Chat_Dialog();
    void addChatUSerList();
    void applyTheme(const QString &theme);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::Chat_Dialog *ui;
    bool _b_loading;
    class AnimatedIconButton *m_fileButton = nullptr;

    QString username;
    ServerConfig serverConfig;
    QString currentPeer;
    QString Send_data;
    QTcpSocket *socket;
    bool offlineNotified = false;
    QString m_theme = QStringLiteral("dark");
    QAction *m_searchAction = nullptr;
    QAction *m_clearAction = nullptr;
    QTimer *m_friendRefreshTimer = nullptr;
    QTimer *m_heartbeatTimer = nullptr;
    QHash<QString, QString> m_peerAvatar;
    QHash<QString, ChatMessageListModel *> m_conversationModels;
    QHash<QString, QString> m_draftCache;
    ChatStorage m_storage;
    QString m_dbPath;

    QListView *m_messageView = nullptr;
    ChatMessageListModel *m_messageModel = nullptr;
    ChatMessageListModel *m_emptyMessageModel = nullptr;
    ChatMessageDelegate *m_messageDelegate = nullptr;

    QVector<QString> Other_IP;
    void setupModernMessageView();
    void initializeStorage();
    ChatMessageListModel *loadConversationModel(const QString &peerName);
    void initializeConnection();
    void loadFriendList();
    void addChatUserItem(const QString &name, const QString &head, const QString &preview, bool online, bool selectByDefault = false);
    void addMessageBubble(const QString &sender, const QString &text, bool outgoing, const QDateTime &timestamp = QDateTime());
    void addSystemBubble(const QString &text, const QDateTime &timestamp = QDateTime());
    qint64 appendPeerMessage(const QString &peerName, const QString &sender, const QString &text,
                             bool outgoing, bool system = false,
                             const QDateTime &timestamp = QDateTime::currentDateTime());
    qint64 appendPeerFileMessage(const QString &peerName, const QString &sender, const QString &fileName,
                                 qint64 fileSize, bool outgoing, bool downloadable,
                                 const QString &localPath = QString(), bool isImage = false,
                                 const QByteArray &thumbnailData = QByteArray());
    void updatePeerFileProgress(const QString &peerName, qint64 messageId, qint64 progress, bool completed, bool downloadable, const QString &localPath = QString());
    void updatePeerImageData(const QString &peerName, qint64 messageId, bool isImage,
                             const QByteArray &thumbnailData, const QString &localPath = QString());
    void requestThumbnailGeneration(const QString &peerName, qint64 messageId, const QString &localPath);
    void addFileBubble(const QString &sender, const QString &fileName, qint64 fileSize, qint64 progress, bool outgoing, bool completed, bool downloadable, const QDateTime &timestamp);
    void renderConversation(const QString &peerName);
    void clearCurrentConversation();
    QString sendFriendCommand(const QString &command);
    void updateCurrentPeer(const QString &peerName);
    void notifyOffline();
    void animateListItem();
    void refreshMessageListLayout();
    void scrollMessagesAnimated();
    void setupIconButtons();
    void updateActionIcons();
    void updateDecorativeIcons();
    void sendOfflineAck(qint64 offlineId);
    void sendHeartbeat();

    // 发送文件状态
    QFile m_sendFile;
    QString m_sendFilePath;
    qint64 m_sendFileSize = 0;
    qint64 m_sendFileSent = 0;
    qint64 m_sendHeaderBytesRemaining = 0;
    static constexpr qint64 SEND_CHUNK = 65536; // 64KB
    void sendNextChunk();

    // 接收文件状态
    QByteArray m_recvBuf;
    bool m_recvFile = false;
    QFile m_inFile;
    qint64 m_fileExp = 0;
    qint64 m_fileGot = 0;
    QString m_inFileName;
    QString m_inFileSender;
    QString m_incomingPeer;
    QString m_targetSavePath; // 用户最终选择的保存路径
    qint64 m_currentRecvMessageId = -1;
    qint64 m_currentSendMessageId = -1;
    QString m_currentSendPeer;

    struct PendingIncomingFile {
        QString sender;
        QString receiver;
        QString fileName;
        qint64 fileSize = 0;
    };
    QHash<qint64, PendingIncomingFile> m_pendingIncomingFiles;

    void processPendingData();
    bool consumeFileBytes();
    void resetIncomingFileState();

private slots:
//    void slot_loading_chat_user();
    void on_chat_edit_textChanged();
    void on_send_btn_clicked();
    void on_readyRead();
    void on_disconnected();
    void onChatUserItemClicked(QListWidgetItem *item);
    void onAddFriendClicked();
    
    // 文件收发槽
    void onFileClicked();
    void onBytesSent(qint64 bytes);
    void onDownloadClicked(int row);
    void onImageClicked(int row);
    void showFeatureComingSoon(const QString &featureName);
};

#endif // CHAT_DIALOG_H
