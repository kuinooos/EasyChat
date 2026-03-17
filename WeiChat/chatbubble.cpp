#include "chatbubble.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QVariant>
#include <QFont>
#include <QFontMetrics>

ChatBubble::ChatBubble(BubbleType type, const QString &sender, const QString &text, QWidget *parent)
    : QWidget(parent), m_type(type)
{
    setObjectName("chat_bubble_container");
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 8, 12, 8);
    root->setSpacing(4);

    m_senderLabel = new QLabel(sender, this);
    m_senderLabel->setObjectName("chat_bubble_sender");
    {
        QFont senderFont(QStringLiteral("Microsoft YaHei UI"), 11, QFont::Medium);
        senderFont.setStyleStrategy(QFont::PreferAntialias);
        m_senderLabel->setFont(senderFont);
    }
    m_senderLabel->setVisible(!sender.isEmpty() && type != System);

    m_textLabel = new QLabel(text, this);
    m_textLabel->setObjectName("chat_bubble_text");
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextFormat(Qt::PlainText);
    {
        QFont bodyFont(QStringLiteral("Microsoft YaHei UI"), 12, QFont::Normal);
        bodyFont.setStyleStrategy(QFont::PreferAntialias);
        m_textLabel->setFont(bodyFont);
    }

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

void ChatBubble::setContentMaxWidth(int maxWidth)
{
    const int safeMax = qMax(120, maxWidth);
    const int innerMax = qMax(80, safeMax - 24);

    int textWidth = 0;
    if (m_textLabel) {
        const QFontMetrics metrics(m_textLabel->font());
        const QStringList lines = m_textLabel->text().split('\n');
        for (const QString &line : lines) {
            textWidth = qMax(textWidth, metrics.horizontalAdvance(line));
        }
    }

    int senderWidth = 0;
    if (m_senderLabel && m_senderLabel->isVisible()) {
        const QFontMetrics metrics(m_senderLabel->font());
        senderWidth = metrics.horizontalAdvance(m_senderLabel->text());
    }

    const int longestLine = qMax(textWidth, senderWidth);
    const int innerWidth = qBound(80, longestLine, innerMax);
    const int bubbleWidth = innerWidth + 24;

    setFixedWidth(bubbleWidth);
    if (m_senderLabel) {
        m_senderLabel->setFixedWidth(innerWidth);
    }
    if (m_textLabel) {
        m_textLabel->setFixedWidth(innerWidth);
    }

    updateGeometry();
    adjustSize();
}
