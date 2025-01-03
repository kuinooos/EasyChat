#ifndef FRIENDTASK_H
#define FRIENDTASK_H
#include <QTcpSocket>
#include <QRunnable>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QThread>
#include <QEventLoop>
#include <QCoreApplication>
#include <QDateTime>
#include <QUuid>
#include <QScopedPointer>
#include <QHostAddress>

class FriendTask : public QObject , public QRunnable{
    Q_OBJECT
public:
    FriendTask(QTcpSocket* socket)
        : m_socket(socket) {
        // 数据库连接的初始化
        setupDatabaseConnection();
        connect(m_socket, &QTcpSocket::disconnected, this, &FriendTask::handleSocketDisconnect);
    }

    ~FriendTask(){
        qDebug() << "当前线程结束";
        cleanupDatabaseConnection();
    }

    void run() override {
        QEventLoop loop;
        // 连接相关信号到事件循环的退出操作等，比如和 m_socket 的 disconnected 信号关联
        connect(m_socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);

        connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
            QByteArray data = m_socket->readAll();
            QString message(data);
            QStringList parts = message.split("::");

            if (parts.isEmpty()) return;

            QString messageType = parts[0];
            if (messageType == "ONLINE") {
                username = parts[1];
                qDebug() << "User online:" << username;
            } else if (messageType == "ADD") {
                QString friendname = parts[1];
                qDebug() << "Add friend request for:" << friendname;
                QString reply = handleFriendADD(friendname);
                m_socket->write(reply.toUtf8());
            } else if (messageType == "FRIEND") {
                QString username_as_friendid = parts[1];
                qDebug() << "Friend list requested for:" << username_as_friendid;
                QStringList reply = handleusersDisplayFRIEND(username_as_friendid);
                QString replyString = reply.join(",");
                qDebug() << username_as_friendid <<"_Friends:" <<replyString;//要返回的好友列表
                m_socket->write(replyString.toUtf8());
            } else if(messageType == "ARGEE"){
                QString friendid=parts[1];
                qDebug() <<"这是你同意添加的好友名称: "<<friendid;
                QString reply = handleFriendARGEE(friendid);
                m_socket->write(reply.toUtf8());
            }else if(messageType == "REFUSE"){
                QString friendid=parts[1];
                qDebug() <<"这是你拒绝添加的好友名称: "<<friendid;
                QString reply = handleFriendREFUSE(friendid);
                m_socket->write(reply.toUtf8());
            }else if(messageType == "NOWMYFRIEND"){
                QString username_as_friendid = parts[1];
                qDebug() << username_as_friendid << ":这是你的好友列表 " ;
                QStringList reply = handleusersFRIENDNOW(username_as_friendid);
                QString replyString = reply.join(",");
                qDebug() << username_as_friendid <<"Friends:" <<replyString;//要返回的好友列表
                m_socket->write(replyString.toUtf8());
            }else if(messageType == "FRIENDINFO"){
                QString friendid = parts[1];
                qDebug() << "请求端口号";
                QString reply = handleGETINFO(friendid);
                qDebug() << "IP及端口号:" << reply;
                m_socket->write(reply.toUtf8());
            }else if(parts.size()<2){
                qWarning() << "Invalid message format:" << message;
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

private slots:
    void handleSocketDisconnect() {
        QString clientIdentifier = getClientIdentifier(m_socket);
        // 通知服务器类（假设可以获取到服务器类实例的指针或者有合适的信号槽机制来通知）移除对应的套接字
        emit socketDisconnected(clientIdentifier);
    }

private:
    QTcpSocket* m_socket;  // 存储传入的套接字
    QString username;
    QSqlDatabase db;  // 数据库连接对象
    QString dbNAME;

    QString getClientIdentifier(QTcpSocket* socket) {
            QHostAddress clientAddress = socket->peerAddress();
            quint16 clientPort = socket->peerPort();
            return QString("%1:%2").arg(clientAddress.toString()).arg(clientPort);
        }
    QString handleFriendADD(const QString& friendname) {
        QSqlQuery query(db);
        query.prepare("SELECT 1 FROM client WHERE username=:username");
        query.bindValue(":username", friendname);

        if (!query.exec() || !query.next()) {
            query.clear();
            return "未找到此人";
        }

        // Check if already friends
        query.prepare("SELECT 1 FROM friend_relationship WHERE (user_id=:userid AND friend_id=:friendid) OR (user_id=:friendid AND friend_id=:userid)");
        query.bindValue(":userid", username);
        query.bindValue(":friendid", friendname);

        if (!query.exec() || query.next()) {
            query.clear();
            return "用户已经是你的好友了";
        }

        // Add friend
        query.prepare("INSERT INTO friend_relationship (user_id, friend_id, status) VALUES (:user_id, :friend_id, 0)");
        query.bindValue(":user_id", username);
        query.bindValue(":friend_id", friendname);

        if (!query.exec()) {
            qWarning() << "Insert failed:" << query.lastError().text();
            query.clear();
            return "未知错误,添加失败";
        }
        query.clear();
        return "已发送请求";
    }
    QString handleFriendARGEE(const QString& friendid){
        QSqlQuery query(db);
        query.prepare("UPDATE friend_relationship SET status=1 WHERE friend_id=:friendid and user_id=:userid");
        query.bindValue(":friendid",username);
        query.bindValue(":userid",friendid);

        if(!query.exec()){
            qWarning() << "改变status失败" << query.lastError().text();
            query.clear();
            return "未知错误";
        }else{
            query.clear();
            return "添加成功";
        }
    }

    QString handleFriendREFUSE(const QString& friendid){
        QSqlQuery query(db);
        query.prepare("DELETE FROM friend_relationship WHERE user_id=:friendid and friend_id=:userid");
        query.bindValue(":friendid",username);
        query.bindValue(":userid",friendid);

        if (!query.exec()) {
            qWarning() << "删除失败" << query.lastError().text();
            query.clear();
            return "未知错误";
        }else{
            query.clear();
            return "已拒绝";
        }

    }


    //展示已有好友
    QStringList handleusersFRIENDNOW(const QString& username){
        QSqlQuery query(db);
        query.prepare("SELECT user_id,friend_id FROM friend_relationship WHERE (friend_id=:friend_id or user_id=:user_id) and status=1");
        query.bindValue(":friend_id", username);
        query.bindValue(":user_id",username);

        QSqlQuery queryONLINE(db);
        QStringList friendIds;
        if (query.exec()) {
            while (query.next()) {
                if(query.value(0).toString()!=username){
                queryONLINE.prepare("SELECT is_online FROM client WHERE username=:username");
                queryONLINE.bindValue(":username",query.value(0).toString());

                if (queryONLINE.exec() && queryONLINE.next()) {
                    friendIds.append(query.value(0).toString()+"+"+queryONLINE.value(0).toString());
                    }
                }

                if(query.value(1).toString()!=username){
                queryONLINE.prepare("SELECT is_online FROM client WHERE username=:username");
                queryONLINE.bindValue(":username",query.value(1).toString());
                if (queryONLINE.exec() && queryONLINE.next()) {
                    friendIds.append(query.value(1).toString()+"+"+queryONLINE.value(0).toString());
                    }
                }
            }
        } else {
            qWarning() << "Query failed:" << query.lastError().text();
        }

        query.clear();
        queryONLINE.clear();
        return friendIds;
    }

    //展示待添加的好友
    QStringList handleusersDisplayFRIEND(const QString& username) {
        QSqlQuery query(db);
        query.prepare("SELECT user_id FROM friend_relationship WHERE friend_id=:friend_id and status=0");
        query.bindValue(":friend_id", username);

        QStringList userIds;
        if (query.exec()) {
            while (query.next()) {
                userIds.append(query.value(0).toString());
            }
        } else {
            qWarning() << "Query failed:" << query.lastError().text();
        }

        query.clear();
        return userIds;
    }

    QString handleGETINFO(QString friendid){
        QSqlQuery query(db);
        query.prepare("SELECT ip,port FROM client WHERE username=:friendid");
        query.bindValue(":friendid",friendid);

        QString reply="";
        if(query.exec()){
            while(query.next()){
                reply.append(query.value(0).toString());
                reply.append("::");
                reply.append(query.value(1).toString());
            }
        }else{
            qWarning() << "Query failed:" << query.lastError().text();
        }

        query.clear();
        reply.remove("::ffff:");
        return reply;
    }
signals:
    void socketDisconnected(QString clientIdentifier);
};

#endif
