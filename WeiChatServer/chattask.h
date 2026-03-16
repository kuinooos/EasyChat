#ifndef CHATTASK_H
#define CHATTASK_H
#pragma once

#include<QObject>
#include<QTcpSocket>
#include<QSql>
#include<QSqlDatabase>
#include<QFile>
#include<QDir>

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
                     const QString &fileName, qint64 fileSize, const QString &filePath);

private slots:
    void onReadyRead();
    void handleSocketDisconnect();
    void onBytesWritten(qint64 bytes);

private:
    void processPendingData();
    void processControlMessage(const QString &message);
    bool consumeFileBytes();
    void finalizeIncomingFile();
    void writeNextFileChunk();

    static QString generateUniqueConnectionName();
    static QSqlDatabase openDatabase(QString &connectionName);
    static void closeDatabase(QSqlDatabase &db, const QString &connectionName);
    QString buildTempFilePath(const QString &incomingFileName) const;
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
    qint64 m_outFileSize = 0;
    qint64 m_outFileSent = 0;
    static constexpr qint64 OUT_CHUNK = 65536;

signals:
    void finished();
};

#endif // CHATTASK_H
