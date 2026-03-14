#pragma once
#include<QTcpServer>
#include<QSet>
#include<QTcpSocket>
#include<QHostAddress>
#include<QtNetwork>
#include"chattask.h"
#include<QPointer>
#include<QReadWriteLock>
#include<QFile>
#include<QThread>

class ChatServer : public QTcpServer
{
        Q_OBJECT

public:
    explicit ChatServer(QObject *parent=nullptr) : QTcpServer(parent){
        clientTaskMap = new QMap<QString, QPointer<ChatTask>>();

        if(this->listen(QHostAddress::AnyIPv6,7777)){
            qDebug() << "IPv6 chat server started on port 7777" << serverAddress().toString();
        }else{
            qDebug() << "Failed to start IPv6 chat server!";
        }

    
    };

    ~ChatServer() override {
        /*找出当前对象（chatserver）的所有子线程并请求它们退出
        当 ChatServer 被销毁时，Qt 的对象树会自动删除所有子对象（包括 QThread）
        如果线程此时还在运行（比如正在执行某个任务）
        强行销毁线程对象会导致未定义行为，通常就是程序直接崩溃。*/
        const auto threads = findChildren<QThread*>();
        for (QThread *thread : threads) {
            thread->quit();
            thread->wait(3000);
        }
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        QThread *thread = new QThread(this);
        ChatTask* task = new ChatTask(socketDescriptor);
        task->moveToThread(thread);

        connect(task,&ChatTask::sendMessageToClient,this,&ChatServer::handledMessage);
        connect(task,&ChatTask::sendFileToClient,this,&ChatServer::handleFile);
        connect(task,&ChatTask::socketDisconnected,this,&ChatServer::onClientDisconnected);
        connect(task,&ChatTask::userOnline,this,&ChatServer::onUserOnline);
        connect(task, &ChatTask::finished, thread, &QThread::quit);
        connect(task, &ChatTask::finished, task, &QObject::deleteLater);
        connect(thread, &QThread::started, task, &ChatTask::start);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);

        connect(thread, &QThread::finished, this, [this, task]() {
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

        thread->start();//触发线程的 started 信号，进而调用 task 的 start 方法，完成套接字的初始化和信号连接
    }

private:
    QMap<QString, QPointer<ChatTask>> *clientTaskMap;

    QReadWriteLock mapLock;

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
