#include <QCoreApplication>
#include "registerserver.h"
#include "LoginServer.h"
#include "offline.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    RegisterServer Rserver;
    if (Rserver.isListening()) {
        qDebug() << "RegisterServer is listening on port 12345" << Rserver.serverAddress().toString();
    } else {
        qDebug() << "RegisterServer failed to listen";
    }

    Server server;
    if(server.isListening()){
        qDebug() << "LoginServer is listening on port 55555" << server.serverAddress().toString();
    } else {
        qDebug() << "LoginServer failed to listen";
    }

    Offline OFFserver;
    if (OFFserver.isListening()) {
        qDebug() << "OfflineServer is listening on port 7776" << OFFserver.serverAddress().toString();
    } else {
        qDebug() << "OfflineServer failed to listen";
    }

    return a.exec();  // 进入事件循环
}
