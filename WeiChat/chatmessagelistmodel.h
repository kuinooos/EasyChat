#ifndef CHATMESSAGELISTMODEL_H
#define CHATMESSAGELISTMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QPixmap>
#include <QVector>

class ChatMessageListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    struct MessageItem {
        enum MessageType {
            Text,
            System,
            File
        };

        MessageType type = Text;
        QString sender;
        QString text;
        bool outgoing = false;
        bool system = false;
        QDateTime timestamp;
        QString fileName;
        qint64 fileSize = 0;
        qint64 fileProgress = 0;
        bool fileCompleted = false;
        bool downloadable = false;
        qint64 messageId = -1;
        QString localPath;
        bool isImage = false;
        QByteArray thumbnailData;
        QPixmap thumbnailPixmap;
    };

    enum MessageRoles {
        SenderRole = Qt::UserRole + 1,
        TextRole,
        OutgoingRole,
        SystemRole,
        TypeRole,
        TimestampRole,
        FileNameRole,
        FileSizeRole,
        FileProgressRole,
        FileCompletedRole,
        DownloadableRole,
        MessageIdRole,
        LocalPathRole,
        IsImageRole,
        ThumbnailDataRole,
        ThumbnailPixmapRole
    };

    explicit ChatMessageListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    int appendMessage(const MessageItem &item);
    bool updateMessage(int row, const MessageItem &item);
    MessageItem messageAt(int row) const;
    int findRowByMessageId(qint64 messageId) const;

private:
    QVector<MessageItem> m_items;
};

#endif // CHATMESSAGELISTMODEL_H
