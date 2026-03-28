#ifndef CHATTASK_H
#define CHATTASK_H
#pragma once

#include<QObject>
#include<QTcpSocket>
#include<QSql>
#include<QSqlDatabase>
#include<QFile>
#include<QDir>
#include<QVector>
#include<QFutureWatcher>
#include<QStandardPaths>
#include<QJsonDocument>
#include<QJsonObject>

class ChatServer;

class ChatTask : public QObject{
     Q_OBJECT
public:
    explicit ChatTask(qintptr socketDescriptor, ChatServer* router, QObject *parent = nullptr);
    ~ChatTask();

public slots:
    void start();
    void deliverMessage(const QString &sender, const QString &message);
    void deliverFile(const QString &sender, const QString &receiver,
                     const QString &fileName, qint64 fileSize, const QString &filePath, bool deleteAfter = true);

private slots:
    void onReadyRead();
    void handleSocketDisconnect();
    void onBytesWritten(qint64 bytes);

private:
    struct OfflineMessageRecord {
        qint64 id = -1;
        QString sender;
        QString content;
        QString sendTimeText;
    };

    void processPendingData();
    void processControlMessage(const QString &message);
    bool consumeFileBytes();
    void finalizeIncomingFile();
    void writeNextFileChunk();

    static QString generateUniqueConnectionName();
    static bool ensureOfflineMessageSchema(QSqlDatabase &db);
    static bool updateUserOnlineStatus(const QString &username, int status);
    static QVector<OfflineMessageRecord> loadUnreadOfflineMessages(QSqlDatabase &db, const QString &receiver,
                                                                   int offset = 0, int limit = 50);
    static bool markOfflineMessageRead(QSqlDatabase &db, qint64 offlineId, const QString &receiver);

    void deliverPendingOfflineMessages();
    void fetchOfflineBatchAsync(int offset);
    void deliverOfflineMessage(qint64 offlineId, const QString &sender, const QString &content, const QString &sendTimeText);
    QString buildTempFilePath(const QString &incomingFileName) const;
    static QString buildOfflineFileStoragePath(const QString &receiver, const QString &fileName);
    void resetIncomingFileState(bool removeTempFile);
    void resetOutgoingFileState(bool removeTemp);

    void handleSendMessage(const QString &sender, const QString &receiver, const QString &content);
    void handleSendFile(const QString &sender, const QString &receiver,
                        const QString &fileName, qint64 fileSize, const QString &filePath);

public:
    static void persistOfflineMessageAsync(const QString &sender, const QString &receiver, const QString &message);
    static void persistOfflineFileAsync(const QString &sender, const QString &receiver, const QString &filePath);

private:
    qintptr socketDescriptor;
    QTcpSocket* m_socket = nullptr;
    ChatServer* m_router = nullptr;
    QString username;

    // 接收状态
    QByteArray pendingBuffer;
    qint64 fileExpectedSize = 0;
    qint64 fileReceivedSize = 0;
    bool receivingFile = false;
    QString fileSender;
    QString fileReceiver;
    QString fileName;
    QString tempFilePath;
    QFile currentFile;

    // 发送状态
    QFile m_outFile;
    QString m_outFilePath;
    QString m_outFileName;
    QString m_outSender;
    QString m_outReceiver;
    qint64 m_outFileSize = 0;
    qint64 m_outFileSent = 0;
    qint64 m_outHeaderBytesRemaining = 0;
    bool m_outTransferStarted = false;
    bool m_outFileShouldDelete = true;  // 发送完是否删除文件（临时文件删除，离线文件保留）
    static constexpr qint64 OUT_CHUNK = 65536;

signals:
    void finished();
};

#endif // CHATTASK_H
