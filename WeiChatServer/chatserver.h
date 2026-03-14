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
        if (workerCount <= 0) workerCount = 4; // 兜底
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
        // 通知所有 worker 线程退出事件循环并等待
        for (WorkerThread *w : m_workers) {
            w->quit();
            w->wait(3000);
        }
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        // 选择负载最小的 worker 线程
        WorkerThread *worker = leastLoadedWorker();

        ChatTask* task = new ChatTask(socketDescriptor);
        worker->incrementCount();

        // 信号-槽连接（task 此时还在主线程，可以安全 connect）
        connect(task,&ChatTask::sendMessageToClient,this,&ChatServer::handledMessage);
        connect(task,&ChatTask::sendFileToClient,this,&ChatServer::handleFile);
        connect(task,&ChatTask::socketDisconnected,this,&ChatServer::onClientDisconnected);
        connect(task,&ChatTask::userOnline,this,&ChatServer::onUserOnline);

        // task 完成时：减少计数、清理 map、销毁 task
        connect(task, &ChatTask::finished, this, [this, worker, task]() {
            worker->decrementCount();

            QWriteLocker locker(&mapLock);
            auto it = clientTaskMap->begin();
            while (it != clientTaskMap->end()) {
                if (it.value() == task) {
                    it = clientTaskMap->erase(it);
                } else {
                    ++it;
                }
            }
        });
        connect(task, &ChatTask::finished, task, &QObject::deleteLater);

        // 将 task 移入 worker 线程，并在其事件循环中启动
        task->moveToThread(worker);
        QMetaObject::invokeMethod(task, "start", Qt::QueuedConnection);
    }

private:
    QMap<QString, QPointer<ChatTask>> *clientTaskMap;

    QReadWriteLock mapLock;

    QVector<WorkerThread*> m_workers;

    // 从 worker 池中选出当前连接数最少的线程
    WorkerThread* leastLoadedWorker() const {
        WorkerThread *best = m_workers.first();
        for (int i = 1; i < m_workers.size(); ++i) {
            if (m_workers[i]->connectionCount() < best->connectionCount()) {
                best = m_workers[i];
            }
        }
        return best;
    }

    //获取本地ip地址
    QString read_ip_address()
    {
        QString ip_address;
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (int i = 0; i < ipAddressesList.size(); ++i)
        {
            if (ipAddressesList.at(i) != QHostAddress::LocalHost &&  ipAddressesList.at(i).toIPv4Address())
            {
                ip_address = ipAddressesList.at(i).toString();
                qDebug()<<ip_address;  //debug
                //break;
            }
        }
        if (ip_address.isEmpty())
            ip_address = QHostAddress(QHostAddress::LocalHost).toString();
        return ip_address;
    }


private slots:
    void onUserOnline(const QString &username) {
        ChatTask *task = qobject_cast<ChatTask*>(sender());
        if (!task) {
            return;
        }

        //读表前加读写锁
        QWriteLocker locker(&mapLock);
        clientTaskMap->insert(username, task);
    }

    void onClientDisconnected(const QString &clientIdentifier){
        qDebug() << "一个客户端已经从map中移除" << clientIdentifier;

        //从图中移除
        QWriteLocker locker(&mapLock);
        clientTaskMap->remove(clientIdentifier);
    }

    void handledMessage(const QString& sender,const QString& receiver,const QString& message){
        if(message==sender){
            return;
        }

        QPointer<ChatTask> receiverTask;
        {
            QReadLocker locker(&mapLock);
            receiverTask = clientTaskMap->value(receiver);
        }

        if(!receiverTask.isNull()){
            QMetaObject::invokeMethod(receiverTask.data(), "deliverMessage", Qt::QueuedConnection,
                                      Q_ARG(QString, sender),
                                      Q_ARG(QString, message));
            qDebug() << "已发送消息给" << receiver;
        }else{
            ChatTask::persistOfflineMessageAsync(sender, receiver, message);
            qDebug() << "用户不在线，消息转为离线存储" << receiver;
        }
    }

    //发送FILE、文件名、文件大小、文件发送者、文件数据给对应客户端
    void handleFile(const QString& sender,const QString& receiver,const QString& fileName,
                    qint64 fileSize,const QString& filePath){
        QPointer<ChatTask> receiverTask;
        {
            QReadLocker locker(&mapLock);
            receiverTask = clientTaskMap->value(receiver);
        }

        if(!receiverTask.isNull()){
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
};
