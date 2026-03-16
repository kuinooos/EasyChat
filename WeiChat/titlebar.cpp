#include "titlebar.h"
#include "animatediconbutton.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("title_bar");
    setFixedHeight(50);
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(8);

    m_titleLabel = new QLabel(QStringLiteral("WeiChat"), this);
    m_titleLabel->setObjectName("title_label");

    m_minBtn = new AnimatedIconButton(this);
    m_minBtn->setObjectName("title_min_btn");
    m_minBtn->setSvgIcon(":/svg/minimize.svg");
    m_minBtn->setFixedSize(32, 28);
    m_minBtn->setToolTip(QStringLiteral("最小化"));

    m_maxBtn = new AnimatedIconButton(this);
    m_maxBtn->setObjectName("title_max_btn");
    m_maxBtn->setSvgIcon(":/svg/maximize.svg");
    m_maxBtn->setFixedSize(32, 28);
    m_maxBtn->setToolTip(QStringLiteral("最大化/还原"));

    m_closeBtn = new AnimatedIconButton(this);
    m_closeBtn->setObjectName("title_close_btn");
    m_closeBtn->setSvgIcon(":/svg/close.svg");
    m_closeBtn->setFixedSize(32, 28);
    m_closeBtn->setToolTip(QStringLiteral("关闭"));

    m_themeBtn = new AnimatedIconButton(this);
    m_themeBtn->setObjectName("title_theme_btn");
    m_themeBtn->setSvgIcon(":/svg/moon.svg");
    m_themeBtn->setFixedSize(32, 28);
    m_themeBtn->setToolTip(QStringLiteral("切换主题"));

    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_themeBtn);
    layout->addWidget(m_minBtn);
    layout->addWidget(m_maxBtn);
    layout->addWidget(m_closeBtn);

    connect(m_minBtn, &QPushButton::clicked, this, &TitleBar::sigMinimize);
    connect(m_maxBtn, &QPushButton::clicked, this, &TitleBar::sigMaximizeRestore);
    connect(m_closeBtn, &QPushButton::clicked, this, &TitleBar::sigClose);
    connect(m_themeBtn, &QPushButton::clicked, this, &TitleBar::sigToggleTheme);
}

void TitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStart = event->globalPos() - window()->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        const QPoint target = event->globalPos() - m_dragStart;
        window()->move(target);
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}
