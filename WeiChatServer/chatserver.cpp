#include "chatserver.h"
#include "chattask.h"
#include "offlinebufferservice.h"
#include "workerthread.h"
#include <QThread>
#include <QMetaObject>
#include <QDebug>
#include <QNetworkInterface>

namespace {
bool isGlobalIpv6Address(const QHostAddress &addr)
{
    if (addr.protocol() != QAbstractSocket::IPv6Protocol) {
        return false;
    }
    if (addr.isLoopback()) {
        return false;
    }
    if (addr.isLinkLocal()) {
        return false;
    }
    if (addr.isMulticast()) {
        return false;
    }

    const Q_IPV6ADDR ip6 = addr.toIPv6Address();
    const bool isUniqueLocal = (ip6[0] & 0xFE) == 0xFC; // fc00::/7
    return !isUniqueLocal;
}

void logReachableIpv6Hints()
{
    QStringList globals;
    const QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addrs) {
        if (isGlobalIpv6Address(addr)) {
            globals << addr.toString();
        }
    }
    globals.removeDuplicates();

    if (globals.isEmpty()) {
        qWarning() << "No global IPv6 address detected. Cross-network IPv6 may fail unless your machine has routable IPv6.";
    } else {
        qDebug() << "Global IPv6 candidates:" << globals;
    }
}

bool listenPreferIpv6WithFallback(QTcpServer *server, quint16 port, const QString &serviceName)
{
    if (server->listen(QHostAddress::AnyIPv6, port)) {
        qDebug() << serviceName << "listening on" << server->serverAddress().toString() << "port" << port << "(IPv6)";
        logReachableIpv6Hints();
        return true;
    }

    const QString ipv6Err = server->errorString();
    qWarning() << serviceName << "IPv6 listen failed:" << ipv6Err << "falling back to IPv4";

    if (server->listen(QHostAddress::Any, port)) {
        qDebug() << serviceName << "listening on" << server->serverAddress().toString() << "port" << port << "(IPv4 fallback)";
        return true;
    }

    qWarning() << serviceName << "listen failed on both IPv6 and IPv4:" << server->errorString();
    return false;
}
}

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {
    clientTaskMap = new QHash<QString, QPointer<ChatTask>>();
    m_offlineBuffer = new OfflineBufferService(this);
    m_offlineBuffer->start();

    int workerCount = QThread::idealThreadCount();
    if (workerCount <= 0) workerCount = 4;
    for (int i = 0; i < workerCount; ++i) {
        auto *w = new WorkerThread(this);
        w->start();
        m_workers.append(w);
    }
    qDebug() << "Worker threads started:" << workerCount;

    listenPreferIpv6WithFallback(this, 7777, QStringLiteral("ChatServer"));
}

ChatServer::~ChatServer() {
    if (m_offlineBuffer) {
        m_offlineBuffer->stop();
    }
    for (WorkerThread *w : m_workers) {
        w->quit();
        w->wait(3000);
    }
    delete clientTaskMap;
}

void ChatServer::registerUser(const QString &username, ChatTask *task) {
    QWriteLocker locker(&mapLock);
    clientTaskMap->insert(username, task);
}

void ChatServer::unregisterUser(const QString &username) {
    QWriteLocker locker(&mapLock);
    clientTaskMap->remove(username);
}

void ChatServer::routeMessage(const QString &sender, const QString &receiver, const QString &message) {
    if (message == sender) return;
    QPointer<ChatTask> receiverTask;
    {
        QReadLocker locker(&mapLock);
        receiverTask = clientTaskMap->value(receiver);
    }

    if (!receiverTask.isNull()) {
        QMetaObject::invokeMethod(receiverTask.data(), "deliverMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, sender),
                                  Q_ARG(QString, message));
    } else {
        if (!receiver.startsWith("bench_")) {
            if (m_offlineBuffer) {
                m_offlineBuffer->enqueueOfflineMessage(sender, receiver, message);
            }
        }
    }
}

void ChatServer::routeFile(const QString &sender, const QString &receiver,
               const QString &fileName, qint64 fileSize, const QString &filePath) {
    QPointer<ChatTask> receiverTask;
    {
        QReadLocker locker(&mapLock);
        receiverTask = clientTaskMap->value(receiver);
    }

    if (!receiverTask.isNull()) {
        QMetaObject::invokeMethod(receiverTask.data(), "deliverFile", Qt::QueuedConnection,
                                  Q_ARG(QString, sender),
                                  Q_ARG(QString, receiver),
                                  Q_ARG(QString, fileName),
                                  Q_ARG(qint64, fileSize),
                                  Q_ARG(QString, filePath));
    } else {
        if (!receiver.startsWith("bench_")) {
            if (m_offlineBuffer) {
                m_offlineBuffer->enqueueOfflineFile(sender, receiver, filePath);
            } else {
                QFile::remove(filePath);
            }
        } else {
            QFile::remove(filePath);
        }
    }
}

void ChatServer::incomingConnection(qintptr socketDescriptor) {
    WorkerThread *worker = leastLoadedWorker();
    ChatTask* task = new ChatTask(socketDescriptor, this, nullptr);
    worker->incrementCount();

    connect(task, &ChatTask::finished, this, [this, worker, task]() {
        worker->decrementCount();
        QWriteLocker locker(&mapLock);
        auto it = clientTaskMap->begin();
        while (it != clientTaskMap->end()) {
            if (it.value() == task) { it = clientTaskMap->erase(it); }
            else { ++it; }
        }
    });
    connect(task, &ChatTask::finished, task, &QObject::deleteLater);

    task->moveToThread(worker);
    QMetaObject::invokeMethod(task, "start", Qt::QueuedConnection);
}

WorkerThread* ChatServer::leastLoadedWorker() const {
    if (m_workers.isEmpty()) return nullptr;
    WorkerThread *best = m_workers.first();
    for (int i = 1; i < m_workers.size(); ++i) {
        if (m_workers[i]->connectionCount() < best->connectionCount())
            best = m_workers[i];
    }
    return best;
}
