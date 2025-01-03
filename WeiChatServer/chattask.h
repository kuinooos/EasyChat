#ifndef CHATTASK_H
#define CHATTASK_H
#pragma once

#include<QObject>
#include<QRunnable>
#include<QTcpSocket>
#include<QSql>
#include<QSqlDatabase>
#include<QCoreApplication>
#include<QDateTime>
#include<QUuid>
#include<QHostAddress>
#include<QSqlError>
#include<QSqlQuery>

class ChatTask : public QObject,public QRunnable{
     Q_OBJECT
public:
    ChatTask(QTcpSocket* socket)
        : m_socket(socket) {
        // 数据库连接的初始化
        setupDatabaseConnection();
        //断开连接时，通知chatserver中的map
        connect(m_socket, &QTcpSocket::disconnected, this, &ChatTask::handleSocketDisconnect);
    }

    ~ChatTask(){
        qDebug() << "当前线程结束";
        cleanupDatabaseConnection();
    }

protected:
    void run() override {
        QEventLoop loop;

        // 连接相关信号到事件循环的退出操作等，比如和 m_socket 的 disconnected 信号关联
        connect(m_socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);

        connect(m_socket,&QTcpSocket::readyRead,this,[this](){
             currentDateTime = QDateTime::currentDateTime();

             while (m_socket->bytesAvailable() > 0) {
                     if (!receivingFile) {
                         // 普通消息或文件头
                         QByteArray data = m_socket->readAll(); // 按行读取消息（假设消息以换行符分割）
                         QString message(data);
                         QStringList parts = message.split("::");

                         if (parts.isEmpty()) return;

                         if (parts[0] == "ONLINE") {
                             // 用户登录
                             username = parts[1];
                             emit userOnline(username, m_socket);
                         } else if (parts[0] == "FILE") {
                             // 接收到文件头
                             qDebug() << "接收到文件头";
                             fileName = parts.value(1);
                             fileExpectedSize = parts.value(2).toInt(); // 获取文件总大小
                             fileSender = parts.value(3);
                             fileReceiver = parts.value(4);

                             fileBuffer.clear(); // 清空缓冲区
                             fileReceivedSize = 0;
                             receivingFile = true; // 进入文件接收模式
                             qDebug() << "文件总大小：" << fileExpectedSize << "b";
                         } else {
                             // 普通消息
                             qDebug() << "接收到消息";
                             QString sender = parts.value(0);
                             QString receiver = parts.value(1);
                             QString content = parts.mid(2).join("::");
                             qDebug() << "Sender:" << sender;
                             qDebug() << "Receiver:" << receiver;
                             qDebug() << "Content:" << content;

                             handleSendMessage(sender, receiver, content);
                         }
                     } else {
                         // 文件数据接收
                         QByteArray chunk = m_socket->read(qMin<qint64>(m_socket->bytesAvailable(),fileExpectedSize - fileReceivedSize)); // 读取剩余的数据
                         fileBuffer.append(chunk);
                         fileReceivedSize += chunk.size();

                         qDebug() << "已接收：" << fileReceivedSize << "/" << fileExpectedSize;

                         if (fileReceivedSize >= fileExpectedSize) {
                                     // 文件接收完成
                                     receivingFile = false; // 重置状态
                                     qDebug() << "文件接收完成：" << fileName << "文件大小：" << fileBuffer.size();
                                     handleSendFile(fileSender, fileReceiver, fileName,fileExpectedSize,fileBuffer);
                                 }

                     }

        }
        });

        connect(m_socket, &QTcpSocket::disconnected, m_socket, &QTcpSocket::deleteLater);

        loop.exec();  // 启动事件循环，此时 run 方法会阻塞在这里，等待事件触发来推进逻辑
    }

    QString generateUniqueConnectionName() {
           QString processId = QString::number(QCoreApplication::applicationPid());
           QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
           QString uuid = QUuid::createUuid().toString();
           return QString("%1_%2_%3").arg(processId).arg(timestamp).arg(uuid);
       }

    void setupDatabaseConnection() {
        dbNAME = generateUniqueConnectionName();
        QString connectionName = dbNAME;
        if (!QSqlDatabase::contains(connectionName)) {
            db = QSqlDatabase::addDatabase("QODBC", connectionName);
            db.setDatabaseName("Driver={SQL Server};Server=(local);Database=ChatApp;Trusted_Connection=yes;");
            if (!db.open()) {
                qWarning() << "Database connection failed:" << db.lastError().text();
            }
        }
    }
    void cleanupDatabaseConnection() {
        QString connectionName = dbNAME;
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase::removeDatabase(connectionName);
        }
    }
    void handleSendFile(const QString& username,const QString& friendname,const QString& fileName,
                        qint64 fileSize,const QByteArray& file){
        QSqlQuery query(db);
        query.prepare("SELECT is_online FROM client WHERE username=:friendname");
        query.bindValue(":friendname",friendname);

        if(query.exec()){
            if(query.next()){
                int isOnline = query.value(0).toUInt();
                qDebug() << "Is Online:" << isOnline;
                qDebug() << username;

                if(isOnline==1){
                    emit sendFileToClient(username,friendname,fileName,fileSize,file);
                }else{
                    query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
                    query.bindValue(":username",username);
                    query.bindValue(":friendname",friendname);
                    QString base64String = file.toBase64();//将二进制数据转换成Base64字符串
                    query.bindValue(":content",file);
                    query.bindValue(":send_time",currentDateTime);
                }
            }else{
                qDebug() << "No results found for" << friendname;
            }
        }else{
            qDebug() << "查询执行失败" << query.lastError().text();
        }
    }

    void handleSendMessage(const QString& username,const QString& friendname,const QString& message){
        QSqlQuery query(db);
//        QString ip;
//        QString port;
//        //查询好友的ip和端口号
//        query.prepare("SELECT ip,port FROM client WHERE username=:friendname");
//        query.bindValue(":friendname",friendname);
//        if(query.exec()&&query.next()){
//            ip = query.value(0).toString();
//            port = query.value(1).toString();
//        }

        //查看好友是否在线，再选择发送
        query.prepare("SELECT is_online FROM client WHERE username=:friendname");
        query.bindValue(":friendname",friendname);

        if(query.exec()){
            //如果查询执行成功，查询是否有结果
            if(query.next()){
                //获取结果列的值
                int isOnline = query.value(0).toInt();
                qDebug() << "Is Online:" << isOnline;

                if(isOnline==1){
                    //好友在线时直接发送消息给ta
                    emit sendMessageToClient(username,friendname,message);
                }else if(isOnline==0){
                    //当对方不在线时保存消息到数据库中

                    query.prepare("INSERT INTO messages(sender_id,receiver_id,message_text,send_time) VALUES(:username,:friendname,:content,:send_time)");
                    query.bindValue(":username",username);
                    query.bindValue(":friendname",friendname);
                    query.bindValue(":content",message);
                    query.bindValue(":send_time",currentDateTime);

                    if(query.exec()){
                        qDebug() << "离线数据已存入数据库";
                    }else{
                        qDebug() << "数据未存入";
                    }
                }




            }else{
                qDebug() << "No results found for" << friendname;
            }


        }else{
            qDebug() << "查询执行失败" << query.lastError().text();
        }



    }

private slots:
    void handleSocketDisconnect() {
        QString clientIdentifier = username;
        // 通知服务器类（假设可以获取到服务器类实例的指针或者有合适的信号槽机制来通知）移除对应的套接字
        emit socketDisconnected(clientIdentifier);
    }

private:
    QTcpSocket* m_socket;  // 存储传入的套接字

    QDateTime currentDateTime;
    QString username;
    QSqlDatabase db;  // 数据库连接对象
    QString dbNAME;

    QByteArray fileBuffer; // 缓存接收的文件内容
    int fileExpectedSize = 0; // 文件总大小
    int fileReceivedSize = 0; // 已接收的大小
    bool receivingFile = false; // 标志当前是否在接收文件内容
    QString fileSender;
    QString fileReceiver;
    QString fileName;
signals:
    void socketDisconnected(QString clientIdentifier);
    void sendMessageToClient(const QString& sender,const QString& receiver,const QString& message);
    void sendFileToClient(const QString& sender,const QString& receiver,const QString& fileName,
                          qint64 fileSize,const QByteArray& file);
    void userOnline(const QString& username,QTcpSocket* socket);
};


#endif // CHATTASK_H
