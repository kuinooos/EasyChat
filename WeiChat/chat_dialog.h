#ifndef CHAT_DIALOG_H
#define CHAT_DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include "server_config.h"
#include <QFile>
#include <QByteArray>
#include "chatfilewid.h"

class QPushButton;
class QListWidgetItem;
class QCloseEvent;

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

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::Chat_Dialog *ui;
    bool _b_loading;

    QString username;
    ServerConfig serverConfig;
    QString currentPeer;
    QString Send_data;
    QTcpSocket *socket;
    bool offlineNotified = false;

    QVector<QString> Other_IP;
    void initializeConnection();
    void loadFriendList();
    void addChatUserItem(const QString &name, const QString &head, const QString &msg, bool selectByDefault = false);
    QString sendFriendCommand(const QString &command);
    void updateCurrentPeer(const QString &peerName);
    void notifyOffline();

    // 发送文件状态
    QFile m_sendFile;
    QString m_sendFilePath;
    qint64 m_sendFileSize = 0;
    qint64 m_sendFileSent = 0;
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
    QString m_targetSavePath; // 用户最终选择的保存路径
    ChatFileWid *m_currentRecvFileWid = nullptr;
    ChatFileWid *m_currentSendFileWid = nullptr;

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
    void onDownloadClicked(const QString &fileName, qint64 fileSize, const QString &peerName);
};

#endif // CHAT_DIALOG_H
