#ifndef OFFLINE_H
#define OFFLINE_H

#include<QTcpServer>
#include<QtSql>
#include<QTcpSocket>

class Offline : public QTcpServer{
        Q_OBJECT
public:
    explicit Offline(QObject *parent=nullptr) : QTcpServer(parent){
        //创建数据库连接
        setupDatabaseConnection();

        if(this->listen(QHostAddress::Any,7776)){
            qDebug() << "Server started on port 7776";
        }else{
            qDebug() << "Failed to start server!";
        }
    };

    ~Offline(){
        cleanupDatabaseConnection();
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        auto *clientSocket = new QTcpSocket(this);
        clientSocket->setSocketDescriptor(socketDescriptor);

        connect(clientSocket,&QTcpSocket::readyRead,this,[=](){
            QByteArray data = clientSocket->readAll();
            QString message(data);
            qDebug() << "已收到" << message <<"发来的下线消息";

            handleOFFLINE(message);
        });

        connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);

    }

    QString generateUniqueConnectionName() {
           QString processId = QString::number(QCoreApplication::applicationPid());
           QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
           QString uuid = QUuid::createUuid().toString();
           return QString("%1_%2_%3").arg(processId).arg(timestamp).arg(uuid);
       }

    void setupDatabaseConnection() {
        //创建数据库连接名字
        dbNAME = generateUniqueConnectionName();
        QString connectionName = dbNAME;
        if (!QSqlDatabase::contains(connectionName)) {
            //创建数据库连接对象
            db = QSqlDatabase::addDatabase("QODBC", connectionName);
            //指定数据库连接的详细参数
            db.setDatabaseName("Driver={SQL Server};Server=(local);Database=ChatApp;Trusted_Connection=yes;");
            if (!db.open()) {
                qWarning() << "Database connection failed:" << db.lastError().text();
            }else{
                qDebug() << "Database connection successful";
            }
        }

        qDebug() << "Offline的数据库连接已建立";
    }
    void cleanupDatabaseConnection() {
        QString connectionName = dbNAME;
        if (QSqlDatabase::contains(connectionName)) {
            if(db.isOpen()){
                db.close();
            }
            QSqlDatabase::removeDatabase(connectionName);
        }
    }

private:
    QString dbNAME;
    QSqlDatabase db;
    void handleOFFLINE(const QString& userid){
        QSqlQuery query(db);
        query.prepare("UPDATE client SET is_online =0 WHERE username=:userid");
        query.bindValue(":userid",userid);

        if(!query.exec()){
            qDebug() << "用户下线状态修改失败";
        }else{
            qDebug() << "用户下线状态修改成功";
        }
    }
};

#endif // OFFLINE_H
