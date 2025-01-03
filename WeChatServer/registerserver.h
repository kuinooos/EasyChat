#ifndef REGISTERSERVER_H
#define REGISTERSERVER_H

#include<QTcpServer>
#include<QTcpSocket>
#include<QDataStream>
#include<QSqlDatabase>
#include<QThread>
#include<QSqlQuery>
#include<QSqlError>
#include<QHostAddress>
#include<QtNetwork>

class RegisterServer : public QTcpServer{
    Q_OBJECT
public:
    explicit RegisterServer(QObject* parent = nullptr) : QTcpServer(parent){
        setupDatabaseConnection();

        QHostAddress server_ip(read_ip_address());
        listen(QHostAddress::Any,12345);
    };

    ~RegisterServer(){
        cleanupDatabaseConnection();
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override{
        qDebug() << "一个客户端发起注册，处理中" << endl;
        QTcpSocket* clientSocket = new QTcpSocket(this);
        clientSocket->setSocketDescriptor(socketDescriptor);

        QString clientIP = clientSocket->peerAddress().toString();
        QString clientPort = QString::number(clientSocket->peerPort());

        connect(clientSocket,&QTcpSocket::readyRead,this,[=](){
            QByteArray data = clientSocket->readAll();
            QDataStream in(&data, QIODevice::ReadOnly);
            QString username;
            QString password;
            in >> username >> password;
            qDebug() << "用户输入username:" << username << "password:" << password;

            QString result = handleRegister(username, password,clientIP,clientPort);
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
        qDebug()<<"Registerserver的数据库连接已建立";
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

    QString handleRegister(const QString& username,const QString& password,const QString& clientip,
                           const QString& clientport){
        if(username==nullptr||password==nullptr){
           return "failed!";
        }

        QSqlQuery query(db);
        query.prepare("INSERT INTO client(username,password,ip,port) VALUES(:username,:password,:ip,:port)");
        query.bindValue(":username",username);
        query.bindValue(":password",password);
        query.bindValue(":ip",clientip);
        query.bindValue(":port",clientport);
        if(!query.exec()){
            qDebug() << "Failed to insert into client"<< query.lastError().text();
            return "failed!";
        }else{
            qDebug() << "Insert successfully";
            return "Successful!";
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
