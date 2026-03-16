#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "server_config.h"
#include"chat_dialog.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString &username, const ServerConfig &serverConfig, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Qt::Edges resizeEdges(const QPoint &pos) const;
    void updateResizeCursor(const QPoint &pos);
    void performResize(const QPoint &globalPos);
    void applyTheme(const QString &theme);

    Ui::MainWindow *ui;
    Chat_Dialog *_chat;
    class TitleBar *m_titleBar = nullptr;
    bool m_leftPressed = false;
    int m_resizeMargin = 6;
    Qt::Edges m_resizeEdges = Qt::Edges();
    QRect m_resizeStartGeometry;
    QPoint m_resizeStartPos;
    QString m_theme = QStringLiteral("dark");
};
#endif // MAINWINDOW_H
