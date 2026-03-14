#include "server_config.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

namespace {
QString settingsPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("client.ini"));
}
}

ServerConfig ServerConfig::load()
{
    ServerConfig config;
    config.host = QStringLiteral("::1");

    QSettings settings(settingsPath(), QSettings::IniFormat);
    config.host = settings.value(QStringLiteral("server/host"), config.host).toString().trimmed();
    config.loginPort = settings.value(QStringLiteral("ports/login"), config.loginPort).toUInt();
    config.registerPort = settings.value(QStringLiteral("ports/register"), config.registerPort).toUInt();
    config.chatPort = settings.value(QStringLiteral("ports/chat"), config.chatPort).toUInt();
    config.friendPort = settings.value(QStringLiteral("ports/friend"), config.friendPort).toUInt();
    config.offlinePort = settings.value(QStringLiteral("ports/offline"), config.offlinePort).toUInt();
    return config;
}

void ServerConfig::save() const
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("server/host"), host.trimmed());
    settings.setValue(QStringLiteral("ports/login"), loginPort);
    settings.setValue(QStringLiteral("ports/register"), registerPort);
    settings.setValue(QStringLiteral("ports/chat"), chatPort);
    settings.setValue(QStringLiteral("ports/friend"), friendPort);
    settings.setValue(QStringLiteral("ports/offline"), offlinePort);
    settings.sync();
}