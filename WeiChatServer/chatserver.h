#pragma once
#include<QTcpServer>
#include<QSet>
#include<QTcpSocket>
#include<QHostAddress>
#include<QtNetwork>
#include"chattask.h"
#include"workerthread.h"
#include<QPointer>
#include<QReadWriteLock>
#include<QFile>
#include<QThread>
#include<QVector>

class ChatServer : public QTcpServer
{
        Q_OBJECT

public:
    explicit ChatServer(QObject *parent=nullptr) : QTcpServer(parent){
        clientTaskMap = new QMap<QString, QPointer<ChatTask>>();

        // 创建固定数量的 Worker 线程（= CPU 逻辑核心数）
        int workerCount = QThread::idealThreadCount();
        if (workerCount <= 0) workerCount = 4;
        for (int i = 0; i < workerCount; ++i) {
            auto *w = new WorkerThread(this);
            w->start();
            m_workers.append(w);
        }
        qDebug() << "Worker threads started:" << workerCount;

        if(this->listen(QHostAddress::AnyIPv6,7777)){
            qDebug() << "IPv6 chat server started on port 7777" << serverAddress().toString();
        }else{
            qDebug() << "Failed to start IPv6 chat server!";
        }
    };

    ~ChatServer() override {
        for (WorkerThread *w : m_workers) {
            w->quit();
            w->wait(3000);
        }
    }

    // ────────────────────────────────────────────────────────
    // 纯路由接口：ChatServer 只负责"查表 + 转发"，不关心业务细节
    // ────────────────────────────────────────────────────────

    // 注册上线用户
    void registerUser(const QString &username, ChatTask *task) {
        QWriteLocker locker(&mapLock);
        clientTaskMap->insert(username, task);
    }

    // 注销离线用户
    void unregisterUser(const QString &username) {
        qDebug() << "一个客户端已经从map中移除" << username;
        QWriteLocker locker(&mapLock);
        clientTaskMap->remove(username);
    }

    // 路由消息：找到接收方并转发；接收方不在线则异步写库
    void routeMessage(const QString &sender, const QString &receiver, const QString &message) {
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
            qDebug() << "已发送消息给" << receiver;
        } else {
            ChatTask::persistOfflineMessageAsync(sender, receiver, message);
            qDebug() << "用户不在线，消息转为离线存储" << receiver;
        }
    }

    // 路由文件：找到接收方并转发；接收方不在线则异步写库
    void routeFile(const QString &sender, const QString &receiver,
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
            ChatTask::persistOfflineFileAsync(sender, receiver, filePath);
        }
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        WorkerThread *worker = leastLoadedWorker();

        // 将 this（路由器）注入 ChatTask，task 自己向路由器申请转发
        // parent 必须为 nullptr，否则无法 moveToThread
        ChatTask* task = new ChatTask(socketDescriptor, this, nullptr);
        worker->incrementCount();

        // finished：减少线程计数，并确保从路由表里清除（handleSocketDisconnect 通常已处理，这里做兜底）
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

private:
    QMap<QString, QPointer<ChatTask>> *clientTaskMap;
    QReadWriteLock mapLock;
    QVector<WorkerThread*> m_workers;

    WorkerThread* leastLoadedWorker() const {
        WorkerThread *best = m_workers.first();
        for (int i = 1; i < m_workers.size(); ++i) {
            if (m_workers[i]->connectionCount() < best->connectionCount())
                best = m_workers[i];
        }
        return best;
    }
};
