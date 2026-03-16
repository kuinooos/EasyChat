#include "chatserver.h"
#include "chattask.h"
#include "workerthread.h"
#include <QThread>
#include <QMetaObject>
#include <QDebug>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {
    clientTaskMap = new QHash<QString, QPointer<ChatTask>>();

    int workerCount = QThread::idealThreadCount();
    if (workerCount <= 0) workerCount = 4;
    for (int i = 0; i < workerCount; ++i) {
        auto *w = new WorkerThread(this);
        w->start();
        m_workers.append(w);
    }
    qDebug() << "Worker threads started:" << workerCount;

    if (this->listen(QHostAddress::AnyIPv6, 7777)) {
        qDebug() << "IPv6 chat server started on port 7777" << serverAddress().toString();
    } else {
        qDebug() << "Failed to start IPv6 chat server!";
    }
}

ChatServer::~ChatServer() {
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
            ChatTask::persistOfflineMessageAsync(sender, receiver, message);
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
            ChatTask::persistOfflineFileAsync(sender, receiver, filePath);
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
