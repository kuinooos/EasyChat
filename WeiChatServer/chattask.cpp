#include "chattask.h"
#include "chatserver.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QUuid>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrent>

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
    emit finished();
}

void ChatTask::processControlMessage(const QString &message) {
    const QStringList parts = message.split("::");
    if (parts.isEmpty()) return;

    if (parts[0] == "ONLINE") {
        if (parts.size() < 2) return;
        username = parts[1];
        if (m_router) m_router->registerUser(username, this);
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
        QSqlQuery query(db);
        query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
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
        QSqlQuery query(db);
        query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
        query.bindValue(":username", sender); query.bindValue(":friendname", receiver);
        query.bindValue(":content", fileData.toBase64()); query.bindValue(":send_time", QDateTime::currentDateTime());
        if (!query.exec()) qWarning() << "离线文件写入失败" << query.lastError().text();
        closeDatabase(db, connectionName);
        QFile::remove(filePath);
    });
}
