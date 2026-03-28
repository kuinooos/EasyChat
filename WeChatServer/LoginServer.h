#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDataStream>
#include <QHostAddress>
#include <QtNetwork>
#include <QCoreApplication>
#include <QDateTime>
#include <QUuid>

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr) : QTcpServer(parent) {
        QHostAddress hostaddr(read_ip_address());
        Q_UNUSED(hostaddr);
        if (listen(QHostAddress::AnyIPv6, 55555)) {
            qDebug() << "LoginServer started on" << serverAddress().toString() << "port 55555 (IPv6)";
        } else {
            qWarning() << "LoginServer IPv6 listen failed:" << errorString() << "falling back to IPv4";
            if (listen(QHostAddress::Any, 55555)) {
                qDebug() << "LoginServer started on" << serverAddress().toString() << "port 55555 (IPv4 fallback)";
            } else {
                qWarning() << "LoginServer failed to start on both IPv6 and IPv4:" << errorString();
            }
        }
    }

    ~Server() = default;

protected:
    void incomingConnection(qintptr socketDescriptor) override {
         qDebug() << "一个客户端连接成功，登录验证中" << endl;

        auto *clientSocket = new QTcpSocket(this);
        if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor for login request" << socketDescriptor;
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
            const QString result = handleLogin(username, password, clientIP, clientPort);
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

    static QString handleLogin(const QString &username, const QString &password, const QString &ip, const QString &port) {
        QString connectionName;
        QSqlDatabase db = openDatabase(connectionName);
        if (!db.isOpen()) {
            closeDatabase(db, connectionName);
            return "failed";
        }

        QString result = "failed";
        QSqlQuery query(db);
        query.prepare("SELECT 1 FROM client WHERE username = :username AND password = :password");
        query.bindValue(":username", username);
        query.bindValue(":password", password);

        if (query.exec() && query.next()) {
            QSqlQuery updateQuery(db);
            updateQuery.prepare("UPDATE client SET ip = :ip, port = :port, is_online = 1 WHERE username = :username");
            updateQuery.bindValue(":username", username);
            updateQuery.bindValue(":ip", ip);
            updateQuery.bindValue(":port", port);
            if (updateQuery.exec()) {
                result = "successful";
            }
        }

        closeDatabase(db, connectionName);
        return result;
    }

    QString read_ip_address()
    {
        QString ip_address;
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        for (int i = 0; i < ipAddressesList.size(); ++i)
        {
            if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address())
            {
                ip_address = ipAddressesList.at(i).toString();
                qDebug() << ip_address;
            }
        }
        if (ip_address.isEmpty())
            ip_address = QHostAddress(QHostAddress::LocalHost).toString();
        return ip_address;
    }
};
