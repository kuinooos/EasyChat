#include<QTcpServer>
#include<QTcpSocket>
#include<QHostAddress>
#include<QDataStream>
#include<QThread>
#include<QSqlDatabase>
#include<QSqlError>
#include<QSqlQuery>
#include"FriendTask.h"
#include<QThreadPool>
#include<QMap>

class FriendServer : public QTcpServer{
    Q_OBJECT
public:
    explicit FriendServer(QObject* parent = nullptr) : QTcpServer(parent){
        clientSocketMap = new QMap<QString, QTcpSocket*>();

        threadPool = QThreadPool::globalInstance();
        threadPool->setMaxThreadCount(100);  // 设置最大线程数
        if (this->listen(QHostAddress::Any, 7000)) {
            qDebug() << "Server started on port 7000";
        } else {
            qDebug() << "Failed to start server!";
        }
    };
    ~FriendServer() {
            // 清理存储套接字对象的映射表，释放内存
            qDeleteAll(*clientSocketMap);
            clientSocketMap->clear();
            delete clientSocketMap;
        }

protected:
    void incomingConnection(qintptr socketDescriptor) override {
            // 创建一个新的套接字对象并设置描述符
            QTcpSocket* socket = new QTcpSocket();
            if (!socket->setSocketDescriptor(socketDescriptor)) {
                qWarning() << "Failed to set socket descriptor for" << socketDescriptor;
                delete socket;
                return;
            }

            QString clientIdentifier = getClientIdentifier(socket);  // 获取客户端标识
            if (clientSocketMap->contains(clientIdentifier)) {
                // 如果已经存在该客户端对应的套接字，复用它
                socket = clientSocketMap->value(clientIdentifier);
            } else {
                // 如果不存在，将新创建的套接字存入映射表
                clientSocketMap->insert(clientIdentifier, socket);
            }

            //监听到的套接字会放入一个task中
            FriendTask* task = new FriendTask(socket);

            connect(task,&FriendTask::socketDisconnected,this,&FriendServer::onClientDisconnected);

            task->setAutoDelete(true);  // 自动删除任务对象
            threadPool->start(task);    // 将任务交给线程池处理
        }
private:
    QThreadPool* threadPool;
    QMap<QString, QTcpSocket*> *clientSocketMap;

    QString getClientIdentifier(QTcpSocket* socket) {
        // 这里简单示例以客户端的 IP 地址和端口号组合作为标识
        QHostAddress clientAddress = socket->peerAddress();
        quint16 clientPort = socket->peerPort();
        return QString("%1:%2").arg(clientAddress.toString()).arg(clientPort);
    }

private slots:
    void onClientDisconnected(const QString &clientIdentifier){
        qDebug() << "Client disconnected:" << clientIdentifier;

        //从图中移除
        clientSocketMap->remove(clientIdentifier);
    }
};
