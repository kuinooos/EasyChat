#ifndef CHATBUBBLE_H
#define CHATBUBBLE_H

#include <QWidget>

class QLabel;

class ChatBubble : public QWidget
{
    Q_OBJECT
public:
    enum BubbleType { Incoming, Outgoing, System };
    explicit ChatBubble(BubbleType type, const QString &sender, const QString &text, QWidget *parent = nullptr);

    BubbleType bubbleType() const;

private:
    BubbleType m_type;
    QLabel *m_senderLabel = nullptr;
    QLabel *m_textLabel = nullptr;
};

#endif // CHATBUBBLE_H
