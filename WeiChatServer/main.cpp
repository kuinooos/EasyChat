#include <QCoreApplication>
#include <chatserver.h>
#include<friendserver.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ChatServer server;
    if(!server.isListening()){
        qDebug() << "WeiChatServer is listening on port 7777...";
    }

    FriendServer friendserver;
    if(!friendserver.isListening()){
        qDebug() << "FriendServer is listening on port 7000...";
    }

    return a.exec();
}
