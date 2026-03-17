#include "chatstorage.h"

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QUuid>

namespace {
bool hasColumn(QSqlDatabase &db, const QString &table, const QString &column)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString() == column) {
            return true;
        }
    }
    return false;
}
}

ChatStorage::ChatStorage()
{
}

ChatStorage::~ChatStorage()
{
    if (m_db) {
        if (m_db->isOpen()) {
            m_db->close();
        }
        const QString name = m_connectionName;
        delete m_db;
        m_db = nullptr;
        QSqlDatabase::removeDatabase(name);
    }
}

bool ChatStorage::initialize(const QString &ownerUserName)
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);

    m_dbPath = appData + "/EasyChat_" + ownerUserName + ".db";
    m_connectionName = QStringLiteral("easychat_chat_%1").arg(QUuid::createUuid().toString(QUuid::Id128));

    m_db = new QSqlDatabase(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName));
    m_db->setDatabaseName(m_dbPath);
    if (!m_db->open()) {
        return false;
    }

    return ensureSchema();
}

qint64 ChatStorage::addMessage(const QString &owner, const QString &peer, const ChatMessageListModel::MessageItem &item)
{
    if (!isReady()) {
        return -1;
    }

    QSqlQuery query(*m_db);
    query.prepare(
        "INSERT INTO messages (owner, peer, type, sender, text, outgoing, system, timestamp, file_name, file_size, file_progress, file_completed, downloadable, local_path, is_image, thumbnail_blob) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(owner);
    query.addBindValue(peer);
    query.addBindValue(static_cast<int>(item.type));
    query.addBindValue(item.sender);
    query.addBindValue(item.text);
    query.addBindValue(item.outgoing ? 1 : 0);
    query.addBindValue(item.system ? 1 : 0);
    query.addBindValue(item.timestamp.isValid() ? item.timestamp.toString(Qt::ISODate) : QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(item.fileName);
    query.addBindValue(item.fileSize);
    query.addBindValue(item.fileProgress);
    query.addBindValue(item.fileCompleted ? 1 : 0);
    query.addBindValue(item.downloadable ? 1 : 0);
    query.addBindValue(item.localPath);
    query.addBindValue(item.isImage ? 1 : 0);
    query.addBindValue(item.thumbnailData);

    if (!query.exec()) {
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

QVector<ChatMessageListModel::MessageItem> ChatStorage::loadConversation(const QString &owner, const QString &peer, int limit)
{
    QVector<ChatMessageListModel::MessageItem> result;
    if (!isReady()) {
        return result;
    }

    QSqlQuery query(*m_db);
    query.prepare(
        "SELECT id, type, sender, text, outgoing, system, timestamp, file_name, file_size, file_progress, file_completed, downloadable, local_path, is_image, thumbnail_blob "
        "FROM messages WHERE owner = ? AND peer = ? ORDER BY id DESC LIMIT ?");
    query.addBindValue(owner);
    query.addBindValue(peer);
    query.addBindValue(limit);

    if (!query.exec()) {
        return result;
    }

    QVector<ChatMessageListModel::MessageItem> reverse;
    while (query.next()) {
        ChatMessageListModel::MessageItem item;
        item.messageId = query.value(0).toLongLong();
        item.type = static_cast<ChatMessageListModel::MessageItem::MessageType>(query.value(1).toInt());
        item.sender = query.value(2).toString();
        item.text = query.value(3).toString();
        item.outgoing = query.value(4).toInt() != 0;
        item.system = query.value(5).toInt() != 0;
        item.timestamp = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
        item.fileName = query.value(7).toString();
        item.fileSize = query.value(8).toLongLong();
        item.fileProgress = query.value(9).toLongLong();
        item.fileCompleted = query.value(10).toInt() != 0;
        item.downloadable = query.value(11).toInt() != 0;
        item.localPath = query.value(12).toString();
        item.isImage = query.value(13).toInt() != 0;
        item.thumbnailData = query.value(14).toByteArray();
        if (!item.thumbnailData.isEmpty()) {
            item.thumbnailPixmap.loadFromData(item.thumbnailData);
        }
        reverse.push_back(item);
    }

    result.reserve(reverse.size());
    for (int i = reverse.size() - 1; i >= 0; --i) {
        result.push_back(reverse.at(i));
    }
    return result;
}

bool ChatStorage::updateFileProgress(const QString &owner, const QString &peer, qint64 messageId,
                                     qint64 progress, bool completed, bool downloadable, const QString &localPath)
{
    if (!isReady() || messageId < 0) {
        return false;
    }

    QSqlQuery query(*m_db);
    query.prepare(
        "UPDATE messages SET file_progress = ?, file_completed = ?, downloadable = ?, local_path = CASE WHEN ? = '' THEN local_path ELSE ? END "
        "WHERE id = ? AND owner = ? AND peer = ?");
    query.addBindValue(progress);
    query.addBindValue(completed ? 1 : 0);
    query.addBindValue(downloadable ? 1 : 0);
    query.addBindValue(localPath);
    query.addBindValue(localPath);
    query.addBindValue(messageId);
    query.addBindValue(owner);
    query.addBindValue(peer);
    return query.exec();
}

bool ChatStorage::updateImageData(const QString &owner, const QString &peer, qint64 messageId,
                                  bool isImage, const QByteArray &thumbnailData, const QString &localPath)
{
    if (!isReady() || messageId < 0) {
        return false;
    }

    QSqlQuery query(*m_db);
    query.prepare(
        "UPDATE messages SET is_image = ?, thumbnail_blob = ?, local_path = CASE WHEN ? = '' THEN local_path ELSE ? END "
        "WHERE id = ? AND owner = ? AND peer = ?");
    query.addBindValue(isImage ? 1 : 0);
    query.addBindValue(thumbnailData);
    query.addBindValue(localPath);
    query.addBindValue(localPath);
    query.addBindValue(messageId);
    query.addBindValue(owner);
    query.addBindValue(peer);
    return query.exec();
}

bool ChatStorage::clearConversation(const QString &owner, const QString &peer)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery deleteMessages(*m_db);
    deleteMessages.prepare("DELETE FROM messages WHERE owner = ? AND peer = ?");
    deleteMessages.addBindValue(owner);
    deleteMessages.addBindValue(peer);
    if (!deleteMessages.exec()) {
        return false;
    }

    QSqlQuery deleteDraft(*m_db);
    deleteDraft.prepare("DELETE FROM drafts WHERE owner = ? AND peer = ?");
    deleteDraft.addBindValue(owner);
    deleteDraft.addBindValue(peer);
    return deleteDraft.exec();
}

bool ChatStorage::saveDraft(const QString &owner, const QString &peer, const QString &draft)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery query(*m_db);
    query.prepare(
        "INSERT INTO drafts(owner, peer, draft, updated_at) VALUES(?, ?, ?, ?) "
        "ON CONFLICT(owner, peer) DO UPDATE SET draft = excluded.draft, updated_at = excluded.updated_at");
    query.addBindValue(owner);
    query.addBindValue(peer);
    query.addBindValue(draft);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    return query.exec();
}

QString ChatStorage::loadDraft(const QString &owner, const QString &peer) const
{
    if (!isReady()) {
        return QString();
    }

    QSqlQuery query(*m_db);
    query.prepare("SELECT draft FROM drafts WHERE owner = ? AND peer = ?");
    query.addBindValue(owner);
    query.addBindValue(peer);
    if (!query.exec()) {
        return QString();
    }
    if (query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

QString ChatStorage::databasePath() const
{
    return m_dbPath;
}

bool ChatStorage::isReady() const
{
    return m_db && m_db->isOpen();
}

bool ChatStorage::ensureSchema()
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery query(*m_db);
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS messages ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "owner TEXT NOT NULL,"
            "peer TEXT NOT NULL,"
            "type INTEGER NOT NULL DEFAULT 0,"
            "sender TEXT,"
            "text TEXT,"
            "outgoing INTEGER NOT NULL DEFAULT 0,"
            "system INTEGER NOT NULL DEFAULT 0,"
            "timestamp TEXT,"
            "file_name TEXT,"
            "file_size INTEGER NOT NULL DEFAULT 0,"
            "file_progress INTEGER NOT NULL DEFAULT 0,"
            "file_completed INTEGER NOT NULL DEFAULT 0,"
            "downloadable INTEGER NOT NULL DEFAULT 0,"
            "local_path TEXT,"
            "is_image INTEGER NOT NULL DEFAULT 0,"
            "thumbnail_blob BLOB"
            ")")) {
        return false;
    }

    if (!hasColumn(*m_db, QStringLiteral("messages"), QStringLiteral("is_image"))) {
        if (!query.exec("ALTER TABLE messages ADD COLUMN is_image INTEGER NOT NULL DEFAULT 0")) {
            return false;
        }
    }
    if (!hasColumn(*m_db, QStringLiteral("messages"), QStringLiteral("thumbnail_blob"))) {
        if (!query.exec("ALTER TABLE messages ADD COLUMN thumbnail_blob BLOB")) {
            return false;
        }
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_messages_owner_peer_id ON messages(owner, peer, id)")) {
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS drafts ("
            "owner TEXT NOT NULL,"
            "peer TEXT NOT NULL,"
            "draft TEXT,"
            "updated_at TEXT,"
            "PRIMARY KEY(owner, peer)"
            ")")) {
        return false;
    }

    return true;
}
