#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>

class QLabel;
class AnimatedIconButton;

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);

signals:
    void sigMinimize();
    void sigMaximizeRestore();
    void sigClose();
    void sigToggleTheme();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *m_titleLabel = nullptr;
    AnimatedIconButton *m_minBtn = nullptr;
    AnimatedIconButton *m_maxBtn = nullptr;
    AnimatedIconButton *m_closeBtn = nullptr;
    AnimatedIconButton *m_themeBtn = nullptr;
    bool m_dragging = false;
    QPoint m_dragStart;
};

#endif // TITLEBAR_H
