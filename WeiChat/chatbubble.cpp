#include "chatbubble.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QVariant>

ChatBubble::ChatBubble(BubbleType type, const QString &sender, const QString &text, QWidget *parent)
    : QWidget(parent), m_type(type)
{
    setObjectName("chat_bubble_container");
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 8, 12, 8);
    root->setSpacing(4);

    m_senderLabel = new QLabel(sender, this);
    m_senderLabel->setObjectName("chat_bubble_sender");
    m_senderLabel->setVisible(!sender.isEmpty() && type != System);

    m_textLabel = new QLabel(text, this);
    m_textLabel->setObjectName("chat_bubble_text");
    m_textLabel->setWordWrap(true);

    root->addWidget(m_senderLabel);
    root->addWidget(m_textLabel);

    if (m_type == Outgoing) {
        setProperty("bubbleType", QVariant(QStringLiteral("outgoing")));
    } else if (m_type == Incoming) {
        setProperty("bubbleType", QVariant(QStringLiteral("incoming")));
    } else {
        setProperty("bubbleType", QVariant(QStringLiteral("system")));
    }
}

ChatBubble::BubbleType ChatBubble::bubbleType() const
{
    return m_type;
}
