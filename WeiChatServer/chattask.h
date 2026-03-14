#ifndef CHATTASK_H
#define CHATTASK_H
#pragma once

#include<QObject>
#include<QTcpSocket>
#include<QSql>
#include<QSqlDatabase>
#include<QCoreApplication>
#include<QDateTime>
#include<QUuid>
#include<QHostAddress>
#include<QSqlError>
#include<QSqlQuery>
#include<QFile>
#include<QDir>
#include<QThread>
#include<QtConcurrent/QtConcurrent>

// 前向声明，避免 chattask.h ↔ chatserver.h 循环 include
// 方法体中凡是调用 m_router->xxx() 的，统一在 chattask.cpp 里实现
class ChatServer;

class ChatTask : public QObject{
     Q_OBJECT
public:
    // router：注入路由器指针（不拥有所有权，parent 必须为 nullptr 以允许 moveToThread）
    explicit ChatTask(qintptr socketDescriptor, ChatServer* router, QObject *parent = nullptr)
        : QObject(parent), socketDescriptor(socketDescriptor), m_router(router) {
    }

    ~ChatTask(){
        resetIncomingFileState(true);
        resetOutgoingFileState(true);
    }

public slots:
    // 在 Worker 线程中初始化 socket（由 invokeMethod 跨线程调用）
    void start();

    // 由路由器通过 invokeMethod 调用，向本客户端投递消息/文件
    void deliverMessage(const QString &sender, const QString &message);
    void deliverFile(const QString &sender, const QString &receiver,
                     const QString &fileName, qint64 fileSize, const QString &filePath);

private slots:
    void onReadyRead() {
        pendingBuffer.append(m_socket->readAll());
        processPendingData();
    }

    // 实现在 .cpp（需要调用 m_router->unregisterUser）
    void handleSocketDisconnect();

    // 异步文件发送：socket 发出 bytesWritten 后触发，继续发下一片
    void onBytesWritten(qint64 bytes);

private:
    // ── 数据库工具（静态，无需 ChatServer，保持内联）──────────────────
    static QString generateUniqueConnectionName() {
        return QString("%1_%2_%3")
            .arg(QCoreApplication::applicationPid())
            .arg(QDateTime::currentMSecsSinceEpoch())
            .arg(QUuid::createUuid().toString());
    }

    static QSqlDatabase openDatabase(QString &connectionName) {
        connectionName = generateUniqueConnectionName();
        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", connectionName);
        db.setDatabaseName("Driver={SQL Server};Server=(local);Database=ChatApp;Trusted_Connection=yes;");
        if (!db.open()) {
            qWarning() << "Database connection failed:" << db.lastError().text();
        }
        return db;
    }

    static void closeDatabase(QSqlDatabase &db, const QString &connectionName) {
        if (db.isOpen()) db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }

    // ── 接收端：数据解析（无需 ChatServer，保持内联）─────────────────
    void processPendingData() {
        while (true) {
            if (receivingFile) {
                if (!consumeFileBytes()) break;
                continue;
            }
            const int newlineIndex = pendingBuffer.indexOf('\n');
            if (newlineIndex < 0) break;

            QByteArray lineData = pendingBuffer.left(newlineIndex);
            pendingBuffer.remove(0, newlineIndex + 1);
            const QString message = QString::fromUtf8(lineData).trimmed();
            if (!message.isEmpty()) processControlMessage(message);
        }
    }

    // 实现在 .cpp（ONLINE 分支需要调用 m_router->registerUser）
    void processControlMessage(const QString &message);

    bool consumeFileBytes() {
        if (!receivingFile || !currentFile.isOpen()) return false;
        const qint64 remaining = fileExpectedSize - fileReceivedSize;
        if (remaining <= 0) { finalizeIncomingFile(); return true; }

        const qint64 chunkSize = qMin<qint64>(pendingBuffer.size(), remaining);
        if (chunkSize <= 0) return false;

        const QByteArray chunk = pendingBuffer.left(chunkSize);
        pendingBuffer.remove(0, static_cast<int>(chunkSize));
        const qint64 written = currentFile.write(chunk);
        if (written != chunk.size()) {
            qWarning() << "Failed to persist incoming file chunk" << fileName;
            resetIncomingFileState(true);
            return false;
        }
        fileReceivedSize += written;
        qDebug() << "已接收：" << fileReceivedSize << "/" << fileExpectedSize;
        if (fileReceivedSize >= fileExpectedSize) { finalizeIncomingFile(); return true; }
        return !pendingBuffer.isEmpty();
    }

    // 实现在 .cpp（需要调用 m_router->routeFile）
    void finalizeIncomingFile();

    QString buildTempFilePath(const QString &incomingFileName) const {
        const QString safeName = incomingFileName.isEmpty()
            ? QStringLiteral("incoming.bin") : incomingFileName;
        return QDir::temp().filePath(generateUniqueConnectionName() + "_" + safeName);
    }

    void resetIncomingFileState(bool removeTempFile) {
        receivingFile = false;
        fileExpectedSize = 0;
        fileReceivedSize = 0;
        fileSender.clear();
        fileReceiver.clear();
        fileName.clear();
        if (currentFile.isOpen()) currentFile.close();
        if (removeTempFile && !tempFilePath.isEmpty()) QFile::remove(tempFilePath);
        tempFilePath.clear();
        currentFile.setFileName(QString());
    }

    // ── 发送端：异步文件发送辅助（实现在 .cpp）────────────────────────
    void writeNextFileChunk();

    void resetOutgoingFileState(bool removeTemp) {
        if (m_outFile.isOpen()) m_outFile.close();
        if (removeTemp && !m_outFilePath.isEmpty()) QFile::remove(m_outFilePath);
        m_outFilePath.clear();
        m_outFileSize = 0;
        m_outFileSent = 0;
        m_outFile.setFileName(QString());
    }

    // ── 路由请求（实现在 .cpp，调用 m_router->routeXxx）──────────────
    void handleSendMessage(const QString &sender, const QString &receiver, const QString &content);
    void handleSendFile(const QString &sender, const QString &receiver,
                        const QString &fileName, qint64 fileSize, const QString &filePath);

public:
    // 离线存储：静态公开，ChatServer 路由器调用
    static void persistOfflineMessageAsync(const QString &sender, const QString &receiver, const QString &message) {
        QtConcurrent::run([sender, receiver, message]() {
            QString connectionName;
            QSqlDatabase db = openDatabase(connectionName);
            if (!db.isOpen()) { closeDatabase(db, connectionName); return; }

            QSqlQuery query(db);
            query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
            query.bindValue(":username", sender);
            query.bindValue(":friendname", receiver);
            query.bindValue(":content", message);
            query.bindValue(":send_time", QDateTime::currentDateTime());
            if (!query.exec()) qWarning() << "离线消息写入失败" << query.lastError().text();
            closeDatabase(db, connectionName);
        });
    }

    static void persistOfflineFileAsync(const QString &sender, const QString &receiver, const QString &filePath) {
        QtConcurrent::run([sender, receiver, filePath]() {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "无法打开离线文件" << filePath;
                QFile::remove(filePath);
                return;
            }
            const QByteArray fileData = file.readAll();
            file.close();

            QString connectionName;
            QSqlDatabase db = openDatabase(connectionName);
            if (!db.isOpen()) { closeDatabase(db, connectionName); QFile::remove(filePath); return; }

            QSqlQuery query(db);
            query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
            query.bindValue(":username", sender);
            query.bindValue(":friendname", receiver);
            query.bindValue(":content", fileData.toBase64());
            query.bindValue(":send_time", QDateTime::currentDateTime());
            if (!query.exec()) qWarning() << "离线文件写入失败" << query.lastError().text();
            closeDatabase(db, connectionName);
            QFile::remove(filePath);
        });
    }

private:
    qintptr socketDescriptor;
    QTcpSocket* m_socket = nullptr;
    ChatServer* m_router = nullptr;  // 路由器，不拥有所有权

    QString username;

    // 接收文件状态
    QByteArray pendingBuffer;
    qint64 fileExpectedSize = 0;
    qint64 fileReceivedSize = 0;
    bool receivingFile = false;
    QString fileSender;
    QString fileReceiver;
    QString fileName;
    QString tempFilePath;
    QFile currentFile;

    // 发送文件状态（异步分片）
    QFile m_outFile;
    QString m_outFilePath;
    qint64 m_outFileSize = 0;
    qint64 m_outFileSent = 0;
    static constexpr qint64 OUT_CHUNK = 65536; // 每次最多发 64KB

signals:
    // 路由相关信号已全部移除（直接调用 m_router 方法）
    // 仅保留生命周期信号
    void finished();
};

#endif // CHATTASK_H
