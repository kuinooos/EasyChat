#include "chattask.h"
#include "chatserver.h"  // 在 .cpp 中 include，打破 chattask.h ↔ chatserver.h 的循环依赖

// ── 初始化 socket（在 Worker 线程中被 invokeMethod 调用）──────────────
void ChatTask::start() {
    m_socket = new QTcpSocket(this);
    if (!m_socket->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "Failed to set socket descriptor" << socketDescriptor;
        emit finished();
        return;
    }
    connect(m_socket, &QTcpSocket::readyRead,    this, &ChatTask::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ChatTask::handleSocketDisconnect);
    // 异步文件发送回调：每次 socket 真正写出数据后触发
    connect(m_socket, &QAbstractSocket::bytesWritten, this, &ChatTask::onBytesWritten);
}

// ── 向本客户端投递文本消息────────────────────────────────────────────
void ChatTask::deliverMessage(const QString &sender, const QString &message) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) return;
    m_socket->write(("MESSAGE::" + sender + "::" + message + "\n").toUtf8());
}

// ── 向本客户端投递文件（异步分片写，不阻塞 Worker 事件循环）──────────
void ChatTask::deliverFile(const QString &sender, const QString &receiver,
                           const QString &fileName, qint64 fileSize, const QString &filePath) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        QFile::remove(filePath);
        return;
    }

    // 若上一次文件还没发完，先清理（理论上不应重入，但做防御处理）
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

    // 发送文件头（一行协议，轻量级，立刻写出）
    const QString header = "FILE::" + fileName + "::" + QString::number(fileSize)
                         + "::" + sender + "::" + receiver + "\n";
    m_socket->write(header.toUtf8());

    // 发第一片；后续由 onBytesWritten 驱动
    writeNextFileChunk();
}

// ── 异步文件分片发送：每次 bytesWritten 后发下一片────────────────────
void ChatTask::onBytesWritten(qint64 bytes) {
    if (!m_outFile.isOpen()) return;  // 没有正在发送的文件，忽略

    m_outFileSent += bytes;

    if (m_outFileSent >= m_outFileSize || m_outFile.atEnd()) {
        qDebug() << "文件发送完成:" << m_outFilePath;
        resetOutgoingFileState(true);  // 关闭并删除临时文件
        return;
    }

    // 只有当发送缓冲区快排空时再写下一片，防止内存不断堆积
    if (m_socket->bytesToWrite() < OUT_CHUNK * 2) {
        writeNextFileChunk();
    }
}

void ChatTask::writeNextFileChunk() {
    if (!m_outFile.isOpen() || !m_socket) return;
    const QByteArray chunk = m_outFile.read(OUT_CHUNK);
    if (!chunk.isEmpty()) {
        m_socket->write(chunk);
        // write() 立即返回，实际发送由事件循环完成，writeNextChunk 由 bytesWritten 驱动
    }
}

// ── 连接断开处理：直接通知路由器注销──────────────────────────────────
void ChatTask::handleSocketDisconnect() {
    const QString id = username;
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_router) m_router->unregisterUser(id);
    emit finished();
}

// ── 协议解析：处理控制消息，ONLINE 注册、FILE 头、普通消息─────────────
void ChatTask::processControlMessage(const QString &message) {
    const QStringList parts = message.split("::");
    if (parts.isEmpty()) return;

    // 用户上线：直接向路由器注册自己
    if (parts[0] == "ONLINE") {
        if (parts.size() < 2) { qWarning() << "Invalid ONLINE message:" << message; return; }
        username = parts[1];
        if (m_router) m_router->registerUser(username, this);
        return;
    }

    // 文件头：开始接收文件数据
    if (parts[0] == "FILE") {
        if (parts.size() < 5) { qWarning() << "Invalid FILE header:" << message; return; }
        fileName        = parts.value(1);
        fileExpectedSize= parts.value(2).toLongLong();
        fileSender      = parts.value(3);
        fileReceiver    = parts.value(4);
        fileReceivedSize= 0;
        receivingFile   = fileExpectedSize > 0;
        tempFilePath    = buildTempFilePath(fileName);

        currentFile.setFileName(tempFilePath);
        if (!currentFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to open temp file:" << tempFilePath;
            resetIncomingFileState(true);
            return;
        }
        qDebug() << "接收到文件头" << fileName << fileExpectedSize << "bytes";
        if (!receivingFile) { finalizeIncomingFile(); return; }
        if (!pendingBuffer.isEmpty()) consumeFileBytes();
        return;
    }

    // 普通文本消息
    if (parts.size() < 2) { qWarning() << "Invalid message format:" << message; return; }
    const QString sender   = parts.value(0);
    const QString receiver = parts.value(1);
    const QString content  = parts.mid(2).join("::");
    handleSendMessage(sender, receiver, content);
}

// ── 文件接收完毕：交给路由器转发──────────────────────────────────────
void ChatTask::finalizeIncomingFile() {
    currentFile.flush();
    currentFile.close();
    receivingFile = false;
    qDebug() << "文件接收完成：" << fileName << "大小：" << fileReceivedSize;
    handleSendFile(fileSender, fileReceiver, fileName, fileExpectedSize, tempFilePath);
    resetIncomingFileState(false);
}

// ── 路由请求：直接调路由器，无信号中转─────────────────────────────────
void ChatTask::handleSendMessage(const QString &sender, const QString &receiver, const QString &content) {
    if (m_router) m_router->routeMessage(sender, receiver, content);
}

void ChatTask::handleSendFile(const QString &sender, const QString &receiver,
                              const QString &fileName, qint64 fileSize, const QString &filePath) {
    if (m_router) m_router->routeFile(sender, receiver, fileName, fileSize, filePath);
}
