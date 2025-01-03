#pragma once
#include<QTcpServer>
#include<QSet>
#include<QTcpSocket>
#include<QHostAddress>
#include<QtNetwork>
#include<QThreadPool>
#include<QtConcurrent/QtConcurrentMap>
#include"chattask.h"
#include<QTimer>

class ChatServer : public QTcpServer
{
        Q_OBJECT

public:
    explicit ChatServer(QObject *parent=nullptr) : QTcpServer(parent){
        clientSocketMap = new QMap<QString, QTcpSocket*>();

        threadPool = QThreadPool::globalInstance();
        threadPool->setMaxThreadCount(100);  // 设置最大线程数

        if(this->listen(QHostAddress::Any,7777)){
            qDebug() << "Server started on port 7777";
        }else{
            qDebug() << "Failed to start server!";
        }
    };

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        // 创建一个新的套接字对象并设置描述符
        QTcpSocket* socket = new QTcpSocket();
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor for" << socketDescriptor;
            delete socket;
            return;
        }

        //监听到的套接字会放入一个task中
        ChatTask* task = new ChatTask(socket);

        connect(task,&ChatTask::sendMessageToClient,this,&ChatServer::handledMessage);
        connect(task,&ChatTask::sendFileToClient,this,&ChatServer::handleFile);
        connect(task,&ChatTask::socketDisconnected,this,&ChatServer::onClientDisconnected);
        connect(task,&ChatTask::userOnline,this,[this](const QString& username,QTcpSocket* socket){
            clientSocketMap->insert(username,socket);
        });

        task->setAutoDelete(true);  // 自动删除任务对象
        threadPool->start(task);    // 将任务交给线程池处理
    }

private:
    QMap<QString, QTcpSocket*> *clientSocketMap;

    QMutex mapMutex;
    QThreadPool* threadPool;

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

    void onClientDisconnected(const QString &clientIdentifier){
        qDebug() << "一个客户端已经从map中移除" << clientIdentifier;

        //从图中移除
        clientSocketMap->remove(clientIdentifier);
    }

    void handledMessage(const QString& sender,const QString& receiver,const QString& message){
        QMutexLocker locker(&mapMutex);

        if(message==sender){
            return;
        }

        if(clientSocketMap->contains(receiver)){
            QTcpSocket* receiverSocket = clientSocketMap->value(receiver);
            qDebug() <<receiverSocket->peerAddress();
            qDebug() <<receiverSocket->peerPort();
            QString fullMessage = "MESSAGE::"+sender+"::"+message;
            receiverSocket->write(fullMessage.toUtf8());
            qDebug() << "已发送消息给" << receiver;
        }else{
            qDebug() << "没有在map中找到上线好友的socket";
        }
    }

    //发送FILE、文件名、文件大小、文件发送者、文件数据给对应客户端
    void handleFile(const QString& sender,const QString& receiver,const QString& fileName,
                    qint64 fileSize,const QByteArray& fileData){
        QMutexLocker locker(&mapMutex);

        if(clientSocketMap->contains(receiver)){
            QTcpSocket* receiverSocket = clientSocketMap->value(receiver);
            qDebug() << "文件数据大小" <<fileData.size();

            //先发送文件的元数据，包括文件名和文件大小
            // 组装文件头，包含发送者、文件名、文件大小信息
            QString header = "FILE::" + fileName +"::" + QString::number(fileSize) + "::" + sender;
            qDebug() << fileName << "::" << fileSize << "::" << sender;
            receiverSocket->write(header.toUtf8());//先把文件头发送给好友
            receiverSocket->flush();

            QThread::msleep(2);
            //分块发送文件数据给接收端好友
            const qint64 chunkSize = 1024;//每次发送的文件数据大小
            qint64 offset = 0;

            while(offset < fileSize){
                qint64 remainingSize = fileSize - offset;//剩余的要传输的文件大小
                QByteArray chunk = fileData.mid(offset,qMin(chunkSize,remainingSize));//每次发送的文件

                qint64 written = 0;
                while(written < chunk.size()){
                    qint64 bytesWritten = receiverSocket->write(chunk.mid(written));
                    if(bytesWritten == -1){
                        qWarning() <<"发送失败，网络错误";
                        return;
                    }
                    written += bytesWritten;
                }

                receiverSocket->flush();
                offset += chunk.size();//每次发送的文件的起始位置
                qDebug() << "已发送:" << offset << "/" << fileData.size();
            }

        }
    }
};
