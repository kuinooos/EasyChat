#include "animatediconbutton.h"

#include <QPainter>
#include <QPropertyAnimation>
#include <QSvgRenderer>
#include <QMouseEvent>

AnimatedIconButton::AnimatedIconButton(QWidget *parent)
    : QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setCheckable(false);
    setFlat(true);
    setText(QString());
    setFocusPolicy(Qt::NoFocus);

    m_baseColor = QColor(220, 223, 228, 220);
    m_hoverColor = QColor(255, 255, 255, 230);
    m_pressColor = QColor(255, 255, 255, 200);

    m_hoverAnim = new QPropertyAnimation(this, "hoverProgress", this);
    m_hoverAnim->setDuration(200);
    m_hoverAnim->setEasingCurve(QEasingCurve::InOutCubic);

    m_pressAnim = new QPropertyAnimation(this, "pressProgress", this);
    m_pressAnim->setDuration(120);
    m_pressAnim->setEasingCurve(QEasingCurve::OutCubic);
}

AnimatedIconButton::~AnimatedIconButton()
{
    delete m_renderer;
}

void AnimatedIconButton::setSvgIcon(const QString &path)
{
    if (m_svgPath == path) return;
    m_svgPath = path;
    delete m_renderer;
    m_renderer = nullptr;
    update();
}

void AnimatedIconButton::setBaseColor(const QColor &color)
{
    m_baseColor = color;
    update();
}

void AnimatedIconButton::setHoverColor(const QColor &color)
{
    m_hoverColor = color;
    update();
}

void AnimatedIconButton::setPressColor(const QColor &color)
{
    m_pressColor = color;
    update();
}

void AnimatedIconButton::setIconSizePx(int size)
{
    m_iconSize = size;
    update();
}

void AnimatedIconButton::setCornerRadius(int radius)
{
    m_cornerRadius = radius;
    update();
}

qreal AnimatedIconButton::hoverProgress() const
{
    return m_hoverProgress;
}

void AnimatedIconButton::setHoverProgress(qreal value)
{
    m_hoverProgress = value;
    update();
}

qreal AnimatedIconButton::pressProgress() const
{
    return m_pressProgress;
}

void AnimatedIconButton::setPressProgress(qreal value)
{
    m_pressProgress = value;
    update();
}

void AnimatedIconButton::enterEvent(QEvent *event)
{
    if (m_hoverAnim) {
        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hoverProgress);
        m_hoverAnim->setEndValue(1.0);
        m_hoverAnim->start();
    }
    QPushButton::enterEvent(event);
}

void AnimatedIconButton::leaveEvent(QEvent *event)
{
    if (m_hoverAnim) {
        m_hoverAnim->stop();
        m_hoverAnim->setStartValue(m_hoverProgress);
        m_hoverAnim->setEndValue(0.0);
        m_hoverAnim->start();
    }
    QPushButton::leaveEvent(event);
}

void AnimatedIconButton::mousePressEvent(QMouseEvent *event)
{
    if (m_pressAnim) {
        m_pressAnim->stop();
        m_pressAnim->setStartValue(m_pressProgress);
        m_pressAnim->setEndValue(1.0);
        m_pressAnim->start();
    }
    QPushButton::mousePressEvent(event);
}

void AnimatedIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressAnim) {
        m_pressAnim->stop();
        m_pressAnim->setStartValue(m_pressProgress);
        m_pressAnim->setEndValue(0.0);
        m_pressAnim->start();
    }
    QPushButton::mouseReleaseEvent(event);
}

void AnimatedIconButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

    QRectF r = rect();
    QColor bg = mixColor(QColor(255, 255, 255, 20), QColor(255, 255, 255, 60), m_hoverProgress);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(r.adjusted(1, 1, -1, -1), m_cornerRadius, m_cornerRadius);

    QColor iconColor = mixColor(m_baseColor, m_hoverColor, m_hoverProgress);
    iconColor = mixColor(iconColor, m_pressColor, m_pressProgress);

    QImage icon = renderSvg(iconColor);
    if (!icon.isNull()) {
        qreal scale = 1.0 - (0.05 * m_pressProgress);
        QSizeF targetSize(m_iconSize * scale, m_iconSize * scale);
        QPointF center = r.center();
        QRectF target(center.x() - targetSize.width() / 2.0,
                      center.y() - targetSize.height() / 2.0,
                      targetSize.width(),
                      targetSize.height());
        p.drawImage(target, icon);
    }
}

void AnimatedIconButton::ensureRenderer()
{
    if (!m_renderer && !m_svgPath.isEmpty()) {
        m_renderer = new QSvgRenderer(m_svgPath, this);
    }
}

QColor AnimatedIconButton::mixColor(const QColor &a, const QColor &b, qreal t) const
{
    return QColor(
        static_cast<int>(a.red() + (b.red() - a.red()) * t),
        static_cast<int>(a.green() + (b.green() - a.green()) * t),
        static_cast<int>(a.blue() + (b.blue() - a.blue()) * t),
        static_cast<int>(a.alpha() + (b.alpha() - a.alpha()) * t)
    );
}

QImage AnimatedIconButton::renderSvg(const QColor &tint) const
{
    const_cast<AnimatedIconButton *>(this)->ensureRenderer();
    if (!m_renderer) return QImage();

    QImage image(m_iconSize, m_iconSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter p(&image);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    m_renderer->render(&p);
    p.end();

    QImage tinted(m_iconSize, m_iconSize, QImage::Format_ARGB32_Premultiplied);
    tinted.fill(Qt::transparent);
    QPainter tp(&tinted);
    tp.setCompositionMode(QPainter::CompositionMode_Source);
    tp.drawImage(0, 0, image);
    tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tp.fillRect(tinted.rect(), tint);
    tp.end();
    return tinted;
}
