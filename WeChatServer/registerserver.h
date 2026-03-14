#ifndef REGISTERSERVER_H
#define REGISTERSERVER_H

#include<QTcpServer>
#include<QTcpSocket>
#include<QDataStream>
#include<QSqlDatabase>
#include<QSqlQuery>
#include<QSqlError>
#include<QHostAddress>
#include<QtNetwork>
#include<QCoreApplication>
#include<QDateTime>
#include<QUuid>

class RegisterServer : public QTcpServer{
    Q_OBJECT
public:
    explicit RegisterServer(QObject* parent = nullptr) : QTcpServer(parent){
        QHostAddress server_ip(read_ip_address());
        Q_UNUSED(server_ip);
        if (listen(QHostAddress::AnyIPv6,12345)) {
            qDebug() << "IPv6 register server started on port 12345" << serverAddress().toString();
        } else {
            qDebug() << "Failed to start IPv6 register server!";
        }
    };

    ~RegisterServer() = default;

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        qDebug() << "一个客户端发起注册，处理中" << endl;

        auto *clientSocket = new QTcpSocket(this);
        if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor for register request" << socketDescriptor;
            delete clientSocket;
            return;
        }

        connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
            const QByteArray data = clientSocket->readAll();
            QDataStream in(data);
            QString username;
            QString password;
            in >> username >> password;

            const QString clientIP = clientSocket->peerAddress().toString();
            const QString clientPort = QString::number(clientSocket->peerPort());
            const QString result = handleRegister(username, password, clientIP, clientPort);
            clientSocket->write(result.toUtf8());
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

    static QString handleRegister(const QString &username, const QString &password, const QString &clientip,
                                  const QString &clientport) {
        if (username.isEmpty() || password.isEmpty()) {
            return "failed!";
        }

        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        if (!db.isOpen()) {
            closeDatabase(db, connectionName);
            return "failed!";
        }

        QString result = "failed!";
        QSqlQuery query(db);
        query.prepare("INSERT INTO client(username,password,ip,port) VALUES(:username,:password,:ip,:port)");
        query.bindValue(":username", username);
        query.bindValue(":password", password);
        query.bindValue(":ip", clientip);
        query.bindValue(":port", clientport);
        if (query.exec()) {
            result = "Successful!";
        } else {
            qDebug() << "Failed to insert into client" << query.lastError().text();
        }

        closeDatabase(db, connectionName);
        return result;
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
};

#endif // REGISTERSERVER_H
