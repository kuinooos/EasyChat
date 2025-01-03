#include <QCoreApplication>
#include "registerserver.h"
#include "LoginServer.h"
#include "offline.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 创建并启动服务端
    RegisterServer Rserver;
    if (Rserver.isListening()) {
        qDebug() << "RegisterServer is listening on port 55555...";
    }

    Server server;
    if(server.isListening()){
        qDebug() << "LoginServer is listening on port 12345...";
    }

    Offline OFFserver;
    OFFserver.isListening();

    return a.exec();  // 进入事件循环
}
