#pragma once
#include <QTcpServer>
#include <QMap>
#include <QPointer>
#include <QReadWriteLock>
#include <QVector>

class ChatTask;
class WorkerThread;

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer() override;

    void registerUser(const QString &username, ChatTask *task);
    void unregisterUser(const QString &username);
    void routeMessage(const QString &sender, const QString &receiver, const QString &message);
    void routeFile(const QString &sender, const QString &receiver,
                   const QString &fileName, qint64 fileSize, const QString &filePath);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    WorkerThread* leastLoadedWorker() const;

private:
    QMap<QString, QPointer<ChatTask>> *clientTaskMap;
    QReadWriteLock mapLock;
    QVector<WorkerThread*> m_workers;
};
