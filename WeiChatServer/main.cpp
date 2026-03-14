#include <QCoreApplication>
#include <chatserver.h>
#include<friendserver.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ChatServer server;
    if(server.isListening()){
        qDebug() << "ChatServer is listening on port 7777" << server.serverAddress().toString();
    } else {
        qDebug() << "ChatServer failed to listen";
    }

    FriendServer friendserver;
    if(friendserver.isListening()){
        qDebug() << "FriendServer is listening on port 7000" << friendserver.serverAddress().toString();
    } else {
        qDebug() << "FriendServer failed to listen";
    }

    return a.exec();
}
