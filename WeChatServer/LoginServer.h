#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QDataStream>
#include <QHostAddress>
#include <QtNetwork>

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr) : QTcpServer(parent) {
        setupDatabaseConnection();

        QHostAddress hostaddr(read_ip_address());
        // 修改为监听所有地址或指定地址
        listen(QHostAddress::Any, 55555);  // 或者 QHostAddress::LocalHost (127.0.0.1)
    }

    ~Server(){
        cleanupDatabaseConnection();
    }
protected:
    void incomingConnection(qintptr socketDescriptor) override {
         qDebug() << "一个客户端连接成功，登录验证中" << endl;
        //当有客户端连接时自动调用方法
        QTcpSocket* clientSocket = new QTcpSocket(this);
        clientSocket->setSocketDescriptor(socketDescriptor);

        // 获取客户端 IP 和端口
        QString clientIP = clientSocket->peerAddress().toString();
        QString clientPort = QString::number(clientSocket->peerPort());
        qDebug() << "端口号" << clientPort;

        connect(clientSocket, &QTcpSocket::readyRead, this, [=]() {//有信息发过来就会变成可读状态
            QByteArray data = clientSocket->readAll();
            QDataStream in(&data, QIODevice::ReadOnly);

            QString username, password;
            in >> username >> password;

            QString result = handleLogin(username, password,clientIP,clientPort);
            clientSocket->write(result.toUtf8());
            clientSocket->disconnectFromHost();
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
            }
        }
        qDebug() << "LoginServer的数据库连接已建立";
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

    QString handleLogin(const QString& username, const QString& password,const QString& ip,const
                        QString& port) {       
        QSqlQuery query(db);
        query.prepare("SELECT * FROM client WHERE username = :username AND password = :password");
        query.bindValue(":username", username);
        query.bindValue(":password", password);

        if (query.exec() && query.next()) {
            //查询成功后把客户端ip和端口号传入
            query.prepare("UPDATE client set ip=:ip,port=:port,is_online=1 WHERE username = :username");
            query.bindValue(":username", username);
            query.bindValue(":ip",ip);
            query.bindValue(":port",port);
            query.exec();
            qDebug() << "查询到用户" << ip << port;
            return "successful";
        } else {
            return "failed";
        }
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
                ip_address =    ipAddressesList.at(i).toString();
                qDebug()<<ip_address;  //debug
                //break;
            }
        }
        if (ip_address.isEmpty())
            ip_address = QHostAddress(QHostAddress::LocalHost).toString();
        return ip_address;
    }

};
