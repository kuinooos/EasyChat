#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <QString>
#include <QtGlobal>

struct ServerConfig
{
    QString host;
    quint16 loginPort = 55555;
    quint16 registerPort = 12345;
    quint16 chatPort = 7777;
    quint16 friendPort = 7000;
    quint16 offlinePort = 7776;

    static ServerConfig load();
    void save() const;
};

#endif // SERVER_CONFIG_H