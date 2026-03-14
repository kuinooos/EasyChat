#ifndef OFFLINE_H
#define OFFLINE_H

#include<QTcpServer>
#include<QtSql>
#include<QTcpSocket>
#include<QCoreApplication>
#include<QDateTime>
#include<QUuid>

class Offline : public QTcpServer{
        Q_OBJECT
public:
    explicit Offline(QObject *parent=nullptr) : QTcpServer(parent){
        if(this->listen(QHostAddress::AnyIPv6,7776)){
            qDebug() << "IPv6 offline server started on port 7776" << serverAddress().toString();
        }else{
            qDebug() << "Failed to start IPv6 offline server!";
        }
    };

    ~Offline() = default;

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        auto *clientSocket = new QTcpSocket(this);
        if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor for offline request" << socketDescriptor;
            delete clientSocket;
            return;
        }

        connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
            const QString message = QString::fromUtf8(clientSocket->readAll()).trimmed();
            qDebug() << "已收到" << message << "发来的下线消息";
            handleOffline(message);
            clientSocket->disconnectFromHost();
        });
        connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
    }

private:
    static QString generateUniqueConnectionName() {
        const QString processId = QString::number(QCoreApplication::applicationPid());
        const QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
        const QString uuid = QUuid::createUuid().toString();
        return QString("%1_%2_%3").arg(processId).arg(timestamp).arg(uuid);
    }

    static QSqlDatabase openDatabase(QString &connectionName) {
        connectionName = generateUniqueConnectionName();
        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", connectionName);
        db.setDatabaseName("Driver={SQL Server};Server=(local);Database=ChatApp;Trusted_Connection=yes;");
        if (!db.open()) {
            qWarning() << "Database connection failed:" << db.lastError().text();
        }
        return db;
    }

    static void closeDatabase(QSqlDatabase &db, const QString &connectionName) {
        if (db.isOpen()) {
            db.close();
        }
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }

    static void handleOffline(const QString &userid) {
        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        if (!db.isOpen()) {
            closeDatabase(db, connectionName);
            return;
        }

        QSqlQuery query(db);
        query.prepare("UPDATE client SET is_online = 0 WHERE username = :userid");
        query.bindValue(":userid", userid);
        if (!query.exec()) {
            qDebug() << "用户下线状态修改失败";
        } else {
            qDebug() << "用户下线状态修改成功";
        }

        closeDatabase(db, connectionName);
    }
};

#endif // OFFLINE_H
