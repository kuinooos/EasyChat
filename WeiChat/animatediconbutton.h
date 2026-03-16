#ifndef ANIMATEDICONBUTTON_H
#define ANIMATEDICONBUTTON_H

#include <QPushButton>
#include <QColor>

class QSvgRenderer;
class QPropertyAnimation;

class AnimatedIconButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
    Q_PROPERTY(qreal pressProgress READ pressProgress WRITE setPressProgress)
public:
    explicit AnimatedIconButton(QWidget *parent = nullptr);
    ~AnimatedIconButton() override;

    void setSvgIcon(const QString &path);
    void setBaseColor(const QColor &color);
    void setHoverColor(const QColor &color);
    void setPressColor(const QColor &color);
    void setIconSizePx(int size);
    void setCornerRadius(int radius);

    qreal hoverProgress() const;
    void setHoverProgress(qreal value);
    qreal pressProgress() const;
    void setPressProgress(qreal value);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void ensureRenderer();
    QColor mixColor(const QColor &a, const QColor &b, qreal t) const;
    QImage renderSvg(const QColor &tint) const;

    QString m_svgPath;
    QSvgRenderer *m_renderer = nullptr;
    QPropertyAnimation *m_hoverAnim = nullptr;
    QPropertyAnimation *m_pressAnim = nullptr;
    QColor m_baseColor;
    QColor m_hoverColor;
    QColor m_pressColor;
    int m_iconSize = 18;
    int m_cornerRadius = 12;
    qreal m_hoverProgress = 0.0;
    qreal m_pressProgress = 0.0;
};

#endif // ANIMATEDICONBUTTON_H
