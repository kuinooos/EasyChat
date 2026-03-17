#include "chatmessagelistmodel.h"

ChatMessageListModel::ChatMessageListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChatMessageListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_items.size();
}

QVariant ChatMessageListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }

    const MessageItem &item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TextRole:
        return item.text;
    case SenderRole:
        return item.sender;
    case OutgoingRole:
        return item.outgoing;
    case SystemRole:
        return item.system;
    case TypeRole:
        return static_cast<int>(item.type);
    case TimestampRole:
        return item.timestamp;
    case FileNameRole:
        return item.fileName;
    case FileSizeRole:
        return item.fileSize;
    case FileProgressRole:
        return item.fileProgress;
    case FileCompletedRole:
        return item.fileCompleted;
    case DownloadableRole:
        return item.downloadable;
    case MessageIdRole:
        return item.messageId;
    case LocalPathRole:
        return item.localPath;
    case IsImageRole:
        return item.isImage;
    case ThumbnailDataRole:
        return item.thumbnailData;
    case ThumbnailPixmapRole:
        return item.thumbnailPixmap;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatMessageListModel::roleNames() const
{
    return {
        {SenderRole, "sender"},
        {TextRole, "text"},
        {OutgoingRole, "outgoing"},
        {SystemRole, "system"},
        {TypeRole, "type"},
        {TimestampRole, "timestamp"},
        {FileNameRole, "fileName"},
        {FileSizeRole, "fileSize"},
        {FileProgressRole, "fileProgress"},
        {FileCompletedRole, "fileCompleted"},
        {DownloadableRole, "downloadable"},
        {MessageIdRole, "messageId"},
        {LocalPathRole, "localPath"},
        {IsImageRole, "isImage"},
        {ThumbnailDataRole, "thumbnailData"},
        {ThumbnailPixmapRole, "thumbnailPixmap"}
    };
}

void ChatMessageListModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

int ChatMessageListModel::appendMessage(const MessageItem &item)
{
    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.push_back(item);
    endInsertRows();
    return row;
}

bool ChatMessageListModel::updateMessage(int row, const MessageItem &item)
{
    if (row < 0 || row >= m_items.size()) {
        return false;
    }
    m_items[row] = item;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx);
    return true;
}

ChatMessageListModel::MessageItem ChatMessageListModel::messageAt(int row) const
{
    if (row < 0 || row >= m_items.size()) {
        return MessageItem();
    }
    return m_items.at(row);
}

int ChatMessageListModel::findRowByMessageId(qint64 messageId) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).messageId == messageId) {
            return i;
        }
    }
    return -1;
}
