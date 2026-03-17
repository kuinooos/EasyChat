#ifndef CHATSTORAGE_H
#define CHATSTORAGE_H

#include <QString>
#include <QVector>

#include "chatmessagelistmodel.h"

class QSqlDatabase;

class ChatStorage
{
public:
    ChatStorage();
    ~ChatStorage();

    bool initialize(const QString &ownerUserName);
    qint64 addMessage(const QString &owner, const QString &peer, const ChatMessageListModel::MessageItem &item);
    QVector<ChatMessageListModel::MessageItem> loadConversation(const QString &owner, const QString &peer, int limit = 2000);
    bool updateFileProgress(const QString &owner, const QString &peer, qint64 messageId,
                            qint64 progress, bool completed, bool downloadable, const QString &localPath);
    bool updateImageData(const QString &owner, const QString &peer, qint64 messageId,
                         bool isImage, const QByteArray &thumbnailData, const QString &localPath);
    bool clearConversation(const QString &owner, const QString &peer);

    bool saveDraft(const QString &owner, const QString &peer, const QString &draft);
    QString loadDraft(const QString &owner, const QString &peer) const;

    QString databasePath() const;
    bool isReady() const;

private:
    bool ensureSchema();

    QString m_connectionName;
    QString m_dbPath;
    QSqlDatabase *m_db = nullptr;
};

#endif // CHATSTORAGE_H
