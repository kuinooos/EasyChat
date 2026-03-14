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

class ChatTask : public QObject{
     Q_OBJECT
public:
    explicit ChatTask(qintptr socketDescriptor, QObject *parent = nullptr)
        : QObject(parent), socketDescriptor(socketDescriptor) {
    }

    ~ChatTask(){
        resetIncomingFileState(true);
    }

public slots:
    void start() {
        m_socket = new QTcpSocket(this);
        //将底层的 TCP 连接句柄（socketDescriptor）“赋予”给 QTcpSocket 对象。
        if (!m_socket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor for chat connection" << socketDescriptor;
            emit finished();
            return;
        }

        connect(m_socket, &QTcpSocket::readyRead, this, &ChatTask::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &ChatTask::handleSocketDisconnect);
    }

    void deliverMessage(const QString &sender, const QString &message) {
        if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
            return;
        }

        const QString fullMessage = "MESSAGE::" + sender + "::" + message + "\n";
        m_socket->write(fullMessage.toUtf8());
    }

    void deliverFile(const QString &sender, const QString &receiver, const QString &fileName,
                     qint64 fileSize, const QString &filePath) {
        if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
            QFile::remove(filePath);
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "无法读取待转发文件" << filePath;
            QFile::remove(filePath);
            return;
        }

        const QString header = "FILE::" + fileName + "::" + QString::number(fileSize) + "::" + sender + "::" + receiver + "\n";
        m_socket->write(header.toUtf8());

        const qint64 chunkSize = 4096;
        while (!file.atEnd()) {
            const QByteArray chunk = file.read(chunkSize);
            if (chunk.isEmpty()) {
                break;
            }

            qint64 written = 0;
            while (written < chunk.size()) {
                const qint64 bytesWritten = m_socket->write(chunk.mid(written));
                if (bytesWritten <= 0) {
                    file.close();
                    QFile::remove(filePath);
                    return;
                }
                written += bytesWritten;
            }
        }

        file.close();
        QFile::remove(filePath);
        m_socket->flush();
    }

private slots:
    void onReadyRead() {
        pendingBuffer.append(m_socket->readAll());
        processPendingData();
    }

    void handleSocketDisconnect() {
        const QString clientIdentifier = username;
        if (m_socket) {
            m_socket->deleteLater();
            m_socket = nullptr;
        }
        emit socketDisconnected(clientIdentifier);
        emit finished();
    }

private:
    static QString generateUniqueConnectionName() {
           QString processId = QString::number(QCoreApplication::applicationPid());
           QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
           QString uuid = QUuid::createUuid().toString();
           return QString("%1_%2_%3").arg(processId).arg(timestamp).arg(uuid);
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
        if (db.isOpen()) {
            db.close();
        }
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }

    void processPendingData() {
        while (true) {
            if (receivingFile) {
                if (!consumeFileBytes()) {
                    break;
                }
                continue;
            }

            const int newlineIndex = pendingBuffer.indexOf('\n');
            if (newlineIndex < 0) {
                break;
            }

            QByteArray lineData = pendingBuffer.left(newlineIndex);
            pendingBuffer.remove(0, newlineIndex + 1);
            const QString message = QString::fromUtf8(lineData).trimmed();
            if (message.isEmpty()) {
                continue;
            }
            processControlMessage(message);
        }
    }

    void processControlMessage(const QString &message) {
        const QStringList parts = message.split("::");
        if (parts.isEmpty()) {
            return;
        }

        if (parts[0] == "ONLINE") {
            if (parts.size() < 2) {
                qWarning() << "Invalid ONLINE message:" << message;
                return;
            }
            username = parts[1];
            emit userOnline(username);
            return;
        }

        if (parts[0] == "FILE") {
            if (parts.size() < 5) {
                qWarning() << "Invalid FILE header:" << message;
                return;
            }

            fileName = parts.value(1);
            fileExpectedSize = parts.value(2).toLongLong();
            fileSender = parts.value(3);
            fileReceiver = parts.value(4);
            fileReceivedSize = 0;
            receivingFile = fileExpectedSize > 0;
            tempFilePath = buildTempFilePath(fileName);

            currentFile.setFileName(tempFilePath);
            if (!currentFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qWarning() << "Failed to open temp file for incoming data:" << tempFilePath;
                resetIncomingFileState(true);
                return;
            }

            qDebug() << "接收到文件头" << fileName << fileExpectedSize << "bytes";
            if (!receivingFile) {
                finalizeIncomingFile();
                return;
            }
            if (pendingBuffer.size() > 0) {
                consumeFileBytes();
            }
            return;
        }

        if (parts.size() < 2) {
            qWarning() << "Invalid message format:" << message;
            return;
        }

        const QString sender = parts.value(0);
        const QString receiver = parts.value(1);
        const QString content = parts.mid(2).join("::");
        handleSendMessage(sender, receiver, content);
    }

    bool consumeFileBytes() {
        if (!receivingFile || !currentFile.isOpen()) {
            return false;
        }

        const qint64 remaining = fileExpectedSize - fileReceivedSize;
        if (remaining <= 0) {
            finalizeIncomingFile();
            return true;
        }

        const qint64 chunkSize = qMin<qint64>(pendingBuffer.size(), remaining);
        if (chunkSize <= 0) {
            return false;
        }

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

        if (fileReceivedSize >= fileExpectedSize) {
            finalizeIncomingFile();
            return true;
        }

        return !pendingBuffer.isEmpty();
    }

    void finalizeIncomingFile() {
        currentFile.flush();
        currentFile.close();
        receivingFile = false;
        qDebug() << "文件接收完成：" << fileName << "文件大小：" << fileReceivedSize;
        handleSendFile(fileSender, fileReceiver, fileName, fileExpectedSize, tempFilePath);
        resetIncomingFileState(false);
    }

    QString buildTempFilePath(const QString &incomingFileName) const {
        const QString safeName = incomingFileName.isEmpty() ? QStringLiteral("incoming.bin") : incomingFileName;
        return QDir::temp().filePath(generateUniqueConnectionName() + "_" + safeName);
    }

    void resetIncomingFileState(bool removeTempFile) {
        receivingFile = false;
        fileExpectedSize = 0;
        fileReceivedSize = 0;
        fileSender.clear();
        fileReceiver.clear();
        fileName.clear();

        if (currentFile.isOpen()) {
            currentFile.close();
        }

        if (removeTempFile && !tempFilePath.isEmpty()) {
            QFile::remove(tempFilePath);
        }
        tempFilePath.clear();
        currentFile.setFileName(QString());
    }

    void handleSendFile(const QString& username,const QString& friendname,const QString& fileName,
                        qint64 fileSize,const QString& filePath){
        emit sendFileToClient(username,friendname,fileName,fileSize,filePath);
    }

    void handleSendMessage(const QString& username,const QString& friendname,const QString& message){
        emit sendMessageToClient(username,friendname,message);
    }

public:
    static void persistOfflineMessageAsync(const QString &sender, const QString &receiver, const QString &message) {
        QtConcurrent::run([sender, receiver, message]() {
            QString connectionName;
            QSqlDatabase db = openDatabase(connectionName);
            if (!db.isOpen()) {
                closeDatabase(db, connectionName);
                return;
            }

            QSqlQuery query(db);
            query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
            query.bindValue(":username", sender);
            query.bindValue(":friendname", receiver);
            query.bindValue(":content", message);
            query.bindValue(":send_time", QDateTime::currentDateTime());
            if (!query.exec()) {
                qWarning() << "离线消息写入失败" << query.lastError().text();
            }

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
            if (!db.isOpen()) {
                closeDatabase(db, connectionName);
                QFile::remove(filePath);
                return;
            }

            QSqlQuery query(db);
            query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
            query.bindValue(":username", sender);
            query.bindValue(":friendname", receiver);
            query.bindValue(":content", fileData.toBase64());
            query.bindValue(":send_time", QDateTime::currentDateTime());
            if (!query.exec()) {
                qWarning() << "离线文件写入失败" << query.lastError().text();
            }

            closeDatabase(db, connectionName);
            QFile::remove(filePath);
        });
    }
private:
    qintptr socketDescriptor;
    QTcpSocket* m_socket = nullptr;  // 存储传入的套接字

    QString username;

    QByteArray pendingBuffer;
    qint64 fileExpectedSize = 0; // 文件总大小
    qint64 fileReceivedSize = 0; // 已接收的大小
    bool receivingFile = false; // 标志当前是否在接收文件内容
    QString fileSender;
    QString fileReceiver;
    QString fileName;
    QString tempFilePath;
    QFile currentFile;
signals:
    void socketDisconnected(QString clientIdentifier);
    void sendMessageToClient(const QString& sender,const QString& receiver,const QString& message);
    void sendFileToClient(const QString& sender,const QString& receiver,const QString& fileName,
                          qint64 fileSize,const QString& filePath);
    void userOnline(const QString& username);
    void finished();
};


#endif // CHATTASK_H
