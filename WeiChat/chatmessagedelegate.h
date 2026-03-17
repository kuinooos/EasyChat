#ifndef CHATMESSAGEDELEGATE_H
#define CHATMESSAGEDELEGATE_H

#include <QStyledItemDelegate>

class ChatMessageDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatMessageDelegate(QObject *parent = nullptr);

    void setTheme(const QString &theme);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void downloadRequested(int row);
    void imageRequested(int row);

private:
    QString m_theme = QStringLiteral("dark");
};

#endif // CHATMESSAGEDELEGATE_H
