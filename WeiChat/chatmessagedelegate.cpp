#include "chatmessagedelegate.h"
#include "chatmessagelistmodel.h"

#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr qint64 kTimestampGapSecs = 5 * 60;

QColor backgroundColor(bool light, bool outgoing, bool system)
{
    if (system) {
        return light ? QColor(230, 232, 235) : QColor(56, 58, 62);
    }
    if (outgoing) {
        return light ? QColor(57, 107, 255) : QColor(72, 109, 213);
    }
    return light ? QColor(255, 255, 255) : QColor(44, 46, 50);
}

QColor textColor(bool light, bool outgoing, bool system)
{
    if (system) {
        return light ? QColor(50, 52, 56) : QColor(230, 233, 238);
    }
    if (outgoing) {
        return QColor(255, 255, 255);
    }
    return light ? QColor(26, 28, 32) : QColor(236, 239, 244);
}

QString formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString::number(bytes) + QStringLiteral(" B");
    }
    const double kb = bytes / 1024.0;
    if (kb < 1024.0) {
        return QString::number(kb, 'f', 1) + QStringLiteral(" KB");
    }
    const double mb = kb / 1024.0;
    if (mb < 1024.0) {
        return QString::number(mb, 'f', 1) + QStringLiteral(" MB");
    }
    const double gb = mb / 1024.0;
    return QString::number(gb, 'f', 1) + QStringLiteral(" GB");
}

QRect bubbleRectForRow(const QStyleOptionViewItem &option, bool outgoing, bool system,
                       int bubbleWidth, int bubbleHeight, bool showTimestamp)
{
    const QRect rowRect = option.rect.adjusted(10, 4, -10, -4);
    const int topOffset = showTimestamp ? 22 : 4;
    if (system) {
        return QRect(rowRect.center().x() - bubbleWidth / 2, rowRect.top() + topOffset, bubbleWidth, bubbleHeight);
    }
    if (outgoing) {
        return QRect(rowRect.right() - bubbleWidth - 42, rowRect.top() + topOffset, bubbleWidth, bubbleHeight);
    }
    return QRect(rowRect.left() + 42, rowRect.top() + topOffset, bubbleWidth, bubbleHeight);
}

QRect fileDownloadButtonRect(const QRect &bubbleRect)
{
    return QRect(bubbleRect.right() - 82, bubbleRect.bottom() - 34, 72, 24);
}

bool shouldShowTimestamp(const QModelIndex &index, const QDateTime &currentTs)
{
    if (!index.isValid() || !currentTs.isValid()) {
        return false;
    }
    if (index.row() <= 0) {
        return true;
    }

    const QModelIndex prevIndex = index.model()->index(index.row() - 1, 0);
    const QDateTime prevTs = prevIndex.data(ChatMessageListModel::TimestampRole).toDateTime();
    if (!prevTs.isValid()) {
        return true;
    }
    return prevTs.secsTo(currentTs) >= kTimestampGapSecs;
}
}

ChatMessageDelegate::ChatMessageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ChatMessageDelegate::setTheme(const QString &theme)
{
    m_theme = theme;
}

void ChatMessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    const QString sender = index.data(ChatMessageListModel::SenderRole).toString();
    const QString text = index.data(ChatMessageListModel::TextRole).toString();
    const QString fileName = index.data(ChatMessageListModel::FileNameRole).toString();
    const bool outgoing = index.data(ChatMessageListModel::OutgoingRole).toBool();
    const bool system = index.data(ChatMessageListModel::SystemRole).toBool();
    const int type = index.data(ChatMessageListModel::TypeRole).toInt();
    const QDateTime ts = index.data(ChatMessageListModel::TimestampRole).toDateTime();
    const qint64 fileSize = index.data(ChatMessageListModel::FileSizeRole).toLongLong();
    const qint64 fileProgress = index.data(ChatMessageListModel::FileProgressRole).toLongLong();
    const bool fileCompleted = index.data(ChatMessageListModel::FileCompletedRole).toBool();
    const bool downloadable = index.data(ChatMessageListModel::DownloadableRole).toBool();
    const bool isImage = index.data(ChatMessageListModel::IsImageRole).toBool();
    QPixmap thumbnail = qvariant_cast<QPixmap>(index.data(ChatMessageListModel::ThumbnailPixmapRole));
    if (thumbnail.isNull()) {
        const QByteArray thumbData = index.data(ChatMessageListModel::ThumbnailDataRole).toByteArray();
        if (!thumbData.isEmpty()) {
            thumbnail.loadFromData(thumbData);
        }
    }
    const bool light = (m_theme == QStringLiteral("light"));
    const bool fileMessage = (type == static_cast<int>(ChatMessageListModel::MessageItem::File));
    const bool imageMessage = fileMessage && isImage;
    const bool showTimestamp = shouldShowTimestamp(index, ts);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect rowRect = option.rect.adjusted(10, 4, -10, -4);
    const int maxBubbleWidth = qMax(180, static_cast<int>(rowRect.width() * 0.68));

    QFont senderFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::DemiBold);
    QFont textFont(QStringLiteral("Microsoft YaHei UI"), 11, QFont::Medium);
    QFont timeFont(QStringLiteral("Microsoft YaHei UI"), 8);
    QFont titleFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::DemiBold);
    QFontMetrics senderFm(senderFont);
    QFontMetrics textFm(textFont);
    QFontMetrics titleFm(titleFont);
    QFontMetrics timeFm(timeFont);

    int contentHeight = 0;
    int bubbleWidth = 0;
    QRect textBound;
    int imageWidth = 0;
    int imageHeight = 0;

    if (imageMessage) {
        const int maxImageWidth = qMin(220, maxBubbleWidth);
        imageWidth = qBound(120, maxImageWidth, maxImageWidth);
        imageHeight = imageWidth;
        if (!thumbnail.isNull()) {
            const QSize bounded = thumbnail.size().scaled(maxImageWidth, 220, Qt::KeepAspectRatio);
            imageWidth = qBound(120, bounded.width(), maxImageWidth);
            imageHeight = qBound(90, bounded.height(), 220);
        }
        bubbleWidth = imageWidth + 24;
        contentHeight = imageHeight;
        if (!fileName.isEmpty()) {
            contentHeight += textFm.height() + 8;
        }
    } else if (fileMessage) {
        const int fileTitleWidth = qMin(maxBubbleWidth, qMax(180, titleFm.horizontalAdvance(fileName) + 24));
        bubbleWidth = qMin(maxBubbleWidth + 24, fileTitleWidth + 24);
        const int progressHeight = 10;
        const int buttonHeight = downloadable ? 28 : 0;
        contentHeight = titleFm.height() + 8 + textFm.height() + 12 + progressHeight + 8 + buttonHeight;
    } else {
        textBound = textFm.boundingRect(QRect(0, 0, maxBubbleWidth, 10000), Qt::TextWordWrap | Qt::AlignLeft, text);
        bubbleWidth = qBound(140, textBound.width() + 24, maxBubbleWidth + 24);
        contentHeight = textBound.height();
    }

    const int senderHeight = (system || sender.isEmpty()) ? 0 : senderFm.height() + 4;
    const int bubbleHeight = senderHeight + contentHeight + 18;

    const QRect bubbleRect = bubbleRectForRow(option, outgoing, system, bubbleWidth, bubbleHeight, showTimestamp);

    if (showTimestamp) {
        painter->setFont(timeFont);
        painter->setPen(light ? QColor(102, 108, 116) : QColor(138, 144, 154));
        const QString timeText = ts.isValid() ? ts.toString(QStringLiteral("hh:mm")) : QStringLiteral("--:--");
        painter->drawText(QRect(rowRect.left(), rowRect.top(), rowRect.width(), timeFm.height() + 2),
                          Qt::AlignHCenter | Qt::AlignVCenter, timeText);
    }

    if (!system) {
        const int avatarTop = rowRect.top() + (showTimestamp ? 22 : 4);
        const QRect avatarRect(outgoing ? rowRect.right() - 34 : rowRect.left(), avatarTop, 32, 32);
        painter->setBrush(outgoing ? QColor(70, 136, 255) : (light ? QColor(224, 230, 239) : QColor(70, 75, 84)));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(avatarRect);

        const QString avatarText = outgoing ? QStringLiteral("我") : (sender.isEmpty() ? QStringLiteral("?") : sender.left(1));
        painter->setPen(outgoing ? QColor(255, 255, 255) : (light ? QColor(40, 44, 50) : QColor(228, 232, 238)));
        painter->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::Bold));
        painter->drawText(avatarRect, Qt::AlignCenter, avatarText);
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(backgroundColor(light, outgoing, system));
    painter->drawRoundedRect(bubbleRect, 14, 14);

    painter->setPen(textColor(light, outgoing, system));

    int top = bubbleRect.top() + 10;
    if (!system && !sender.isEmpty()) {
        painter->setFont(senderFont);
        painter->drawText(QRect(bubbleRect.left() + 12, top, bubbleRect.width() - 24, senderFm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, sender);
        top += senderFm.height() + 4;
    }

    if (imageMessage) {
        const QRect imageRect(bubbleRect.left() + 12, top, imageWidth, imageHeight);
        painter->setPen(Qt::NoPen);
        painter->setBrush(light ? QColor(240, 243, 248) : QColor(54, 58, 64));
        painter->drawRoundedRect(imageRect, 10, 10);

        if (!thumbnail.isNull()) {
            QPainterPath clipPath;
            clipPath.addRoundedRect(imageRect, 10, 10);
            painter->save();
            painter->setClipPath(clipPath);
            painter->drawPixmap(imageRect, thumbnail.scaled(imageRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            painter->restore();
        } else {
            painter->setPen(light ? QColor(110, 116, 126) : QColor(174, 180, 190));
            painter->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));
            painter->drawText(imageRect, Qt::AlignCenter, QStringLiteral("图片加载中..."));
        }
        top += imageHeight + 6;

        if (!fileName.isEmpty()) {
            painter->setPen(textColor(light, outgoing, system));
            painter->setFont(textFont);
            painter->drawText(QRect(bubbleRect.left() + 12, top, bubbleRect.width() - 24, textFm.height()),
                              Qt::AlignLeft | Qt::AlignVCenter, fileName);
        }
    } else if (fileMessage) {
        painter->setFont(titleFont);
        painter->drawText(QRect(bubbleRect.left() + 12, top, bubbleRect.width() - 24, titleFm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, fileName);
        top += titleFm.height() + 6;

        painter->setFont(textFont);
        const QString progressText = QStringLiteral("%1 / %2").arg(formatSize(fileProgress)).arg(formatSize(fileSize));
        painter->drawText(QRect(bubbleRect.left() + 12, top, bubbleRect.width() - 24, textFm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, progressText);
        top += textFm.height() + 8;

        const QRect progressRect(bubbleRect.left() + 12, top, bubbleRect.width() - 24, 10);
        painter->setPen(Qt::NoPen);
        painter->setBrush(light ? QColor(214, 220, 228) : QColor(79, 84, 93));
        painter->drawRoundedRect(progressRect, 5, 5);

        const int fillWidth = fileSize <= 0 ? 0 : static_cast<int>((progressRect.width() * qMin(fileProgress, fileSize)) / qMax<qint64>(1, fileSize));
        if (fillWidth > 0) {
            painter->setBrush(outgoing ? QColor(181, 219, 255) : QColor(70, 136, 255));
            painter->drawRoundedRect(QRect(progressRect.left(), progressRect.top(), fillWidth, progressRect.height()), 5, 5);
        }
        top += 16;

        if (downloadable) {
            const QRect btnRect = fileDownloadButtonRect(bubbleRect);
            painter->setBrush(light ? QColor(31, 36, 44) : QColor(236, 240, 245));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(btnRect, 8, 8);
            painter->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::DemiBold));
            painter->setPen(light ? QColor(248, 250, 252) : QColor(23, 27, 32));
            painter->drawText(btnRect, Qt::AlignCenter, fileCompleted ? QStringLiteral("另存为") : QStringLiteral("下载"));
        }
    } else {
        painter->setFont(textFont);
        painter->drawText(QRect(bubbleRect.left() + 12, top, bubbleRect.width() - 24, textBound.height() + 4),
                          Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
    }

    painter->restore();
}

QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    const QString sender = index.data(ChatMessageListModel::SenderRole).toString();
    const QString text = index.data(ChatMessageListModel::TextRole).toString();
    const QString fileName = index.data(ChatMessageListModel::FileNameRole).toString();
    const bool system = index.data(ChatMessageListModel::SystemRole).toBool();
    const int type = index.data(ChatMessageListModel::TypeRole).toInt();
    const bool downloadable = index.data(ChatMessageListModel::DownloadableRole).toBool();
    const bool fileCompleted = index.data(ChatMessageListModel::FileCompletedRole).toBool();
    const bool isImage = index.data(ChatMessageListModel::IsImageRole).toBool();
    const QDateTime ts = index.data(ChatMessageListModel::TimestampRole).toDateTime();
    const bool showTimestamp = shouldShowTimestamp(index, ts);
    QPixmap thumbnail = qvariant_cast<QPixmap>(index.data(ChatMessageListModel::ThumbnailPixmapRole));

    const int rowWidth = qMax(260, option.rect.width() > 0 ? option.rect.width() : 420);
    const int maxBubbleWidth = qMax(180, static_cast<int>(rowWidth * 0.68));

    QFont senderFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::DemiBold);
    QFont textFont(QStringLiteral("Microsoft YaHei UI"), 11);
    QFont titleFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::DemiBold);
    QFontMetrics senderFm(senderFont);
    QFontMetrics textFm(textFont);
    QFontMetrics titleFm(titleFont);

    const int senderHeight = (system || sender.isEmpty()) ? 0 : senderFm.height() + 4;
    int contentHeight = 0;
    if (type == static_cast<int>(ChatMessageListModel::MessageItem::File) && isImage) {
        int imageHeight = 180;
        if (!thumbnail.isNull()) {
            const QSize bounded = thumbnail.size().scaled(qMin(220, maxBubbleWidth), 220, Qt::KeepAspectRatio);
            imageHeight = qBound(90, bounded.height(), 220);
        }
        contentHeight = imageHeight + (fileName.isEmpty() ? 0 : textFm.height() + 8);
    } else if (type == static_cast<int>(ChatMessageListModel::MessageItem::File)) {
        contentHeight = titleFm.height() + textFm.height() + 34;
        if (downloadable) {
            contentHeight += 30;
        }
    } else {
        const QRect textBound = textFm.boundingRect(QRect(0, 0, maxBubbleWidth, 10000), Qt::TextWordWrap | Qt::AlignLeft, text);
        contentHeight = textBound.height();
    }
    const int timestampHeight = showTimestamp ? 18 : 0;
    const int totalHeight = senderHeight + contentHeight + 30 + timestampHeight;

    return QSize(rowWidth, qMax(44, totalHeight));
}

bool ChatMessageDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index)
{
    Q_UNUSED(model);
    if (!event || !index.isValid()) {
        return false;
    }

    const int type = index.data(ChatMessageListModel::TypeRole).toInt();
    if (type != static_cast<int>(ChatMessageListModel::MessageItem::File)) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    const bool outgoing = index.data(ChatMessageListModel::OutgoingRole).toBool();
    const bool system = index.data(ChatMessageListModel::SystemRole).toBool();
    const bool downloadable = index.data(ChatMessageListModel::DownloadableRole).toBool();
    const bool completed = index.data(ChatMessageListModel::FileCompletedRole).toBool();
    const bool isImage = index.data(ChatMessageListModel::IsImageRole).toBool();
    const bool hasLocalPath = !index.data(ChatMessageListModel::LocalPathRole).toString().isEmpty();

    if (isImage) {
        if (!completed || !hasLocalPath) {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const int maxBubbleWidth = qMax(180, static_cast<int>(option.rect.adjusted(10, 4, -10, -4).width() * 0.68));
            const int bubbleWidth = qMin(maxBubbleWidth, 220) + 24;
            const int bubbleHeight = 220;
            const QDateTime ts = index.data(ChatMessageListModel::TimestampRole).toDateTime();
            const bool showTimestamp = shouldShowTimestamp(index, ts);
            const QRect bubbleRect = bubbleRectForRow(option, outgoing, system, bubbleWidth, bubbleHeight, showTimestamp);
            if (bubbleRect.contains(mouseEvent->pos())) {
                emit imageRequested(index.row());
                return true;
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    if (!downloadable) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    const int maxBubbleWidth = qMax(180, static_cast<int>(option.rect.adjusted(10, 4, -10, -4).width() * 0.68));
    QFont senderFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::DemiBold);
    QFont titleFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::DemiBold);
    QFontMetrics senderFm(senderFont);
    QFontMetrics titleFm(titleFont);

    const QString sender = index.data(ChatMessageListModel::SenderRole).toString();
    const QString fileName = index.data(ChatMessageListModel::FileNameRole).toString();
    const int senderHeight = (system || sender.isEmpty()) ? 0 : senderFm.height() + 4;
    const int fileTitleWidth = qMin(maxBubbleWidth, qMax(180, titleFm.horizontalAdvance(fileName) + 24));
    const int bubbleWidth = qMin(maxBubbleWidth + 24, fileTitleWidth + 24);
    const int progressAndButtonHeight = downloadable ? 64 : 36;
    const int bubbleHeight = senderHeight + titleFm.height() + senderFm.height() + progressAndButtonHeight;
    const QDateTime ts = index.data(ChatMessageListModel::TimestampRole).toDateTime();
    const bool showTimestamp = shouldShowTimestamp(index, ts);
    const QRect bubbleRect = bubbleRectForRow(option, outgoing, system, bubbleWidth, bubbleHeight, showTimestamp);
    const QRect buttonRect = fileDownloadButtonRect(bubbleRect);

    if (event->type() == QEvent::MouseButtonRelease) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (buttonRect.contains(mouseEvent->pos())) {
            emit downloadRequested(index.row());
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
