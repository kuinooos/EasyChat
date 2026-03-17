#include "chattask.h"
#include "chatserver.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QUuid>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

ChatTask::ChatTask(qintptr socketDescriptor, ChatServer* router, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor), m_router(router) {
}

ChatTask::~ChatTask() {
    resetIncomingFileState(true);
    resetOutgoingFileState(true);
}

void ChatTask::start() {
    m_socket = new QTcpSocket(this);
    if (!m_socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Failed to set socket descriptor" << socketDescriptor;
        emit finished();
        return;
    }
    connect(m_socket, &QTcpSocket::readyRead,    this, &ChatTask::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ChatTask::handleSocketDisconnect);
    connect(m_socket, &QAbstractSocket::bytesWritten, this, &ChatTask::onBytesWritten);
}

void ChatTask::onReadyRead() {
    pendingBuffer.append(m_socket->readAll());
    processPendingData();
}

void ChatTask::processPendingData() {
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

void ChatTask::deliverMessage(const QString &sender, const QString &message) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    m_socket->write(("MESSAGE::" + sender + "::" + message + "\n").toUtf8());
}

void ChatTask::deliverFile(const QString &sender, const QString &receiver,
                           const QString &fileName, qint64 fileSize, const QString &filePath) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        QFile::remove(filePath);
        return;
    }
    resetOutgoingFileState(false);
    m_outFile.setFileName(filePath);
    if (!m_outFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法读取待转发文件" << filePath;
        QFile::remove(filePath);
        return;
    }
    m_outFilePath = filePath;
    m_outFileSize = fileSize;
    m_outFileSent = 0;

    const QString header = "FILE::" + fileName + "::" + QString::number(fileSize)
                         + "::" + sender + "::" + receiver + "\n";
    m_socket->write(header.toUtf8());
    writeNextFileChunk();
}

void ChatTask::onBytesWritten(qint64 bytes) {
    if (!m_outFile.isOpen()) return;
    m_outFileSent += bytes;
    if (m_outFileSent >= m_outFileSize || m_outFile.atEnd()) {
        resetOutgoingFileState(true);
        return;
    }
    if (m_socket->bytesToWrite() < OUT_CHUNK * 2) {
        writeNextFileChunk();
    }
}

void ChatTask::writeNextFileChunk() {
    if (!m_outFile.isOpen() || !m_socket) return;
    const QByteArray chunk = m_outFile.read(OUT_CHUNK);
    if (!chunk.isEmpty()) {
        m_socket->write(chunk);
    }
}

void ChatTask::handleSocketDisconnect() {
    const QString id = username;
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_router) m_router->unregisterUser(id);
    if (!id.isEmpty()) updateUserOnlineStatus(id, 0);
    emit finished();
}

void ChatTask::processControlMessage(const QString &message) {
    const QStringList parts = message.split("::");
    if (parts.isEmpty()) return;

    if (parts[0] == "ONLINE") {
        if (parts.size() < 2) return;
        username = parts[1];
        if (m_router) m_router->registerUser(username, this);
        updateUserOnlineStatus(username, 1);
        deliverPendingOfflineMessages();
        return;
    }

    if (parts[0] == "ACK") {
        if (parts.size() < 2 || username.isEmpty()) return;
        bool ok = false;
        const qint64 offlineId = parts[1].toLongLong(&ok);
        if (!ok || offlineId <= 0) return;

        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        if (!db.isOpen()) {
            closeDatabase(db, connectionName);
            return;
        }
        ensureOfflineMessageSchema(db);
        markOfflineMessageRead(db, offlineId, username);
        closeDatabase(db, connectionName);
        return;
    }

    if (parts[0] == "FILE") {
        if (parts.size() < 5) return;
        fileName = parts.value(1);
        fileExpectedSize = parts.value(2).toLongLong();
        fileSender = parts.value(3);
        fileReceiver = parts.value(4);
        fileReceivedSize = 0;
        receivingFile = fileExpectedSize > 0;
        tempFilePath = buildTempFilePath(fileName);
        currentFile.setFileName(tempFilePath);
        if (!currentFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            resetIncomingFileState(true);
            return;
        }
        if (!receivingFile) { finalizeIncomingFile(); return; }
        if (!pendingBuffer.isEmpty()) consumeFileBytes();
        return;
    }

    if (parts.size() < 2) return;
    const QString sender = parts.value(0);
    const QString receiver = parts.value(1);
    const QString content = parts.mid(2).join("::");
    handleSendMessage(sender, receiver, content);
}

bool ChatTask::consumeFileBytes() {
    if (!receivingFile || !currentFile.isOpen()) return false;
    const qint64 remaining = fileExpectedSize - fileReceivedSize;
    if (remaining <= 0) { finalizeIncomingFile(); return true; }
    const qint64 chunkSize = qMin<qint64>(pendingBuffer.size(), remaining);
    if (chunkSize <= 0) return false;
    const QByteArray chunk = pendingBuffer.left(chunkSize);
    pendingBuffer.remove(0, static_cast<int>(chunkSize));
    const qint64 written = currentFile.write(chunk);
    if (written != chunk.size()) {
        resetIncomingFileState(true);
        return false;
    }
    fileReceivedSize += written;
    if (fileReceivedSize >= fileExpectedSize) { finalizeIncomingFile(); return true; }
    return !pendingBuffer.isEmpty();
}

void ChatTask::finalizeIncomingFile() {
    currentFile.flush();
    currentFile.close();
    receivingFile = false;
    handleSendFile(fileSender, fileReceiver, fileName, fileExpectedSize, tempFilePath);
    resetIncomingFileState(false);
}

QString ChatTask::generateUniqueConnectionName() {
    return QString("%1_%2_%3")
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QUuid::createUuid().toString());
}

QSqlDatabase ChatTask::openDatabase(QString &connectionName) {
    connectionName = generateUniqueConnectionName();
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", connectionName);
    db.setDatabaseName("Driver={SQL Server};Server=(local);Database=ChatApp;Trusted_Connection=yes;");
    if (!db.open()) qWarning() << "Database connection failed:" << db.lastError().text();
    return db;
}

void ChatTask::closeDatabase(QSqlDatabase &db, const QString &connectionName) {
    if (db.isOpen()) db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool ChatTask::ensureOfflineMessageSchema(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (!query.exec(
            "IF COL_LENGTH('messages', 'id') IS NULL "
            "BEGIN "
            "ALTER TABLE messages ADD id BIGINT IDENTITY(1,1) NOT NULL; "
            "END")) {
        qWarning() << "补充 messages.id 字段失败" << query.lastError().text();
        return false;
    }

    if (!query.exec(
            "IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = 'idx_messages_receiver_read_time' AND object_id = OBJECT_ID('messages')) "
            "BEGIN "
            "CREATE INDEX idx_messages_receiver_read_time ON messages(receiver_id, is_read, send_time); "
            "END")) {
        qWarning() << "创建离线消息索引失败" << query.lastError().text();
        return false;
    }

    return true;
}

bool ChatTask::updateUserOnlineStatus(const QString &username, int status)
{
    QString connectionName;
    QSqlDatabase db = openDatabase(connectionName);
    if (!db.isOpen()) {
        closeDatabase(db, connectionName);
        return false;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE client SET is_online = :status WHERE username = :username");
    query.bindValue(":status", status);
    query.bindValue(":username", username);
    const bool ok = query.exec();
    if (!ok) {
        qWarning() << "更新在线状态失败" << username << query.lastError().text();
    }

    closeDatabase(db, connectionName);
    return ok;
}

QVector<ChatTask::OfflineMessageRecord> ChatTask::loadUnreadOfflineMessages(
        QSqlDatabase &db, const QString &receiver, int offset, int limit)
{
    QVector<OfflineMessageRecord> records;
    QSqlQuery query(db);
    // 兼容旧版 SQL Server：使用 ROW_NUMBER 分页，避免 OFFSET/FETCH 语法不支持
    // ensureOfflineMessageSchema 已保证 id 列存在，ORDER BY id 提供稳定分页顺序
    const int startRow = offset + 1;
    const int endRow = offset + limit;
    query.prepare(
        "WITH unread AS ("
        "SELECT id, sender_id, message_text, CONVERT(varchar(23), send_time, 121) AS send_time_text, "
        "ROW_NUMBER() OVER (ORDER BY send_time ASC, id ASC) AS rn "
        "FROM messages WHERE receiver_id = :receiver AND ISNULL(is_read, 0) = 0"
        ") "
        "SELECT id, sender_id, message_text, send_time_text "
        "FROM unread WHERE rn BETWEEN :startRow AND :endRow "
        "ORDER BY rn ASC");
    query.bindValue(":receiver", receiver);
    query.bindValue(":startRow", startRow);
    query.bindValue(":endRow",   endRow);
    if (!query.exec()) {
        qWarning() << "查询离线消息失败 offset=" << offset << query.lastError().text();
        return records;
    }

    while (query.next()) {
        OfflineMessageRecord item;
        item.id           = query.value(0).toLongLong();
        item.sender       = query.value(1).toString();
        item.content      = query.value(2).toString();
        item.sendTimeText = query.value(3).toString();
        records.push_back(item);
    }
    return records;
}

bool ChatTask::markOfflineMessageRead(QSqlDatabase &db, qint64 offlineId, const QString &receiver)
{
    QSqlQuery query(db);
    query.prepare("UPDATE messages SET is_read = 1 WHERE id = :id AND receiver_id = :receiver");
    query.bindValue(":id", offlineId);
    query.bindValue(":receiver", receiver);
    if (!query.exec()) {
        qWarning() << "更新离线消息已读失败" << offlineId << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

void ChatTask::deliverPendingOfflineMessages()
{
    // 入口只做前置检查，真正的 DB 查询交给 fetchOfflineBatchAsync 异步执行
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState || username.isEmpty())
        return;
    fetchOfflineBatchAsync(0);
}

void ChatTask::fetchOfflineBatchAsync(int offset)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState || username.isEmpty())
        return;

    // 每批 50 条；constexpr 在 lambda 体内可直接使用（C++11 起免捕获）
    constexpr int kBatch = 50;
    const QString user = username;   // 按值捕获，防止 this 销毁后悬空引用

    // QFutureWatcher 挂载到 this：ChatTask 销毁时 watcher 自动析构，信号连接自动断开
    auto *watcher = new QFutureWatcher<QVector<OfflineMessageRecord>>(this);

    // finished 信号在 ChatTask 所在的 WorkerThread 事件循环中派发；
    // DB 查询期间事件循环照常运行，可处理该用户的心跳包或实时消息
    connect(watcher, &QFutureWatcher<QVector<OfflineMessageRecord>>::finished,
            this, [this, watcher, offset]() {
        watcher->deleteLater();

        const QVector<OfflineMessageRecord> batch = watcher->result();
        qDebug() << "[离线补发]" << username
                 << "第" << (offset / 50 + 1) << "批，本批" << batch.size() << "条";

        for (const OfflineMessageRecord &item : batch) {
            if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
                return;   // socket 中途断开，终止推送
            deliverOfflineMessage(item.id, item.sender, item.content, item.sendTimeText);
        }

        // 本批恰好满 kBatch 条，说明可能还有更多，继续取下一页
        if (batch.size() == kBatch)
            fetchOfflineBatchAsync(offset + kBatch);
    });

    // 在 Qt 全局线程池中执行 DB 查询，完全不阻塞 WorkerThread 事件循环
    watcher->setFuture(QtConcurrent::run([user, offset]() -> QVector<OfflineMessageRecord> {
        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        QVector<OfflineMessageRecord> records;
        if (!db.isOpen()) {
            closeDatabase(db, connectionName);
            return records;
        }
        ensureOfflineMessageSchema(db);
        records = loadUnreadOfflineMessages(db, user, offset, kBatch);
        closeDatabase(db, connectionName);
        return records;
    }));
}

void ChatTask::deliverOfflineMessage(qint64 offlineId, const QString &sender, const QString &content, const QString &sendTimeText)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    const QByteArray encoded = content.toUtf8().toBase64();
    const QString packet = QStringLiteral("OFFLINE::%1::%2::%3::%4\n")
                               .arg(offlineId)
                               .arg(sender)
                               .arg(QString::fromLatin1(encoded))
                               .arg(sendTimeText);
    m_socket->write(packet.toUtf8());
}

QString ChatTask::buildTempFilePath(const QString &incomingFileName) const {
    const QString safeName = incomingFileName.isEmpty() ? "incoming.bin" : incomingFileName;
    return QDir::temp().filePath(generateUniqueConnectionName() + "_" + safeName);
}

void ChatTask::resetIncomingFileState(bool removeTempFile) {
    receivingFile = false;
    fileExpectedSize = 0;
    fileReceivedSize = 0;
    fileSender.clear(); fileReceiver.clear(); fileName.clear();
    if (currentFile.isOpen()) currentFile.close();
    if (removeTempFile && !tempFilePath.isEmpty()) QFile::remove(tempFilePath);
    tempFilePath.clear();
    currentFile.setFileName(QString());
}

void ChatTask::resetOutgoingFileState(bool removeTemp) {
    if (m_outFile.isOpen()) m_outFile.close();
    if (removeTemp && !m_outFilePath.isEmpty()) QFile::remove(m_outFilePath);
    m_outFilePath.clear(); m_outFileSize = 0; m_outFileSent = 0;
    m_outFile.setFileName(QString());
}

void ChatTask::handleSendMessage(const QString &sender, const QString &receiver, const QString &content) {
    if (m_router) m_router->routeMessage(sender, receiver, content);
}

void ChatTask::handleSendFile(const QString &sender, const QString &receiver,
                              const QString &fileName, qint64 fileSize, const QString &filePath) {
    if (m_router) m_router->routeFile(sender, receiver, fileName, fileSize, filePath);
}

void ChatTask::persistOfflineMessageAsync(const QString &sender, const QString &receiver, const QString &message) {
    QtConcurrent::run([sender, receiver, message]() {
        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        if (!db.isOpen()) { closeDatabase(db, connectionName); return; }
        ensureOfflineMessageSchema(db);
        QSqlQuery query(db);
        query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time,is_read) VALUES(:username,:friendname,:content,:send_time,0)");
        query.bindValue(":username", sender); query.bindValue(":friendname", receiver);
        query.bindValue(":content", message); query.bindValue(":send_time", QDateTime::currentDateTime());
        if (!query.exec()) qWarning() << "离线消息写入失败" << query.lastError().text();
        closeDatabase(db, connectionName);
    });
}

void ChatTask::persistOfflineFileAsync(const QString &sender, const QString &receiver, const QString &filePath) {
    QtConcurrent::run([sender, receiver, filePath]() {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) { QFile::remove(filePath); return; }
        const QByteArray fileData = file.readAll();
        file.close();
        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        if (!db.isOpen()) { closeDatabase(db, connectionName); QFile::remove(filePath); return; }
        ensureOfflineMessageSchema(db);
        QSqlQuery query(db);
        query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time,is_read) VALUES(:username,:friendname,:content,:send_time,0)");
        query.bindValue(":username", sender); query.bindValue(":friendname", receiver);
        query.bindValue(":content", fileData.toBase64()); query.bindValue(":send_time", QDateTime::currentDateTime());
        if (!query.exec()) qWarning() << "离线文件写入失败" << query.lastError().text();
        closeDatabase(db, connectionName);
        QFile::remove(filePath);
    });
}
