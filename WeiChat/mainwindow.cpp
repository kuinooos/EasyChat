#include "mainwindow.h"
#include "ui_mainwindow.h"
#include"chat_dialog.h"
#include "titlebar.h"
#include "animatediconbutton.h"
#include<QObject>
#include<QMouseEvent>
#include<QApplication>
#include<QStyle>
MainWindow::MainWindow(const QString &username, const ServerConfig &serverConfig, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setObjectName("app_root");
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMouseTracking(true);

    _chat=new Chat_Dialog(username, serverConfig, this);

    m_titleBar = new TitleBar(this);
    if (ui->verticalLayout_2) {
        ui->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayout_2->setSpacing(0);
        ui->verticalLayout_2->addWidget(m_titleBar);
        ui->verticalLayout_2->addWidget(_chat);
    } else {
        setCentralWidget(_chat);
    }

    connect(m_titleBar, &TitleBar::sigMinimize, this, &MainWindow::showMinimized);
    connect(m_titleBar, &TitleBar::sigMaximizeRestore, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_titleBar, &TitleBar::sigClose, this, &MainWindow::close);
    connect(m_titleBar, &TitleBar::sigToggleTheme, this, [this]() {
        m_theme = (m_theme == QStringLiteral("dark")) ? QStringLiteral("light") : QStringLiteral("dark");
        applyTheme(m_theme);
    });

    applyTheme(m_theme);
}

MainWindow::~MainWindow()
{
    delete ui;
}

Qt::Edges MainWindow::resizeEdges(const QPoint &pos) const
{
    Qt::Edges edges;
    const QRect r = rect();
    if (pos.x() <= m_resizeMargin) edges |= Qt::LeftEdge;
    if (pos.x() >= r.width() - m_resizeMargin) edges |= Qt::RightEdge;
    if (pos.y() <= m_resizeMargin) edges |= Qt::TopEdge;
    if (pos.y() >= r.height() - m_resizeMargin) edges |= Qt::BottomEdge;
    return edges;
}

void MainWindow::updateResizeCursor(const QPoint &pos)
{
    const Qt::Edges edges = resizeEdges(pos);
    if (edges.testFlag(Qt::TopEdge) && edges.testFlag(Qt::LeftEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges.testFlag(Qt::TopEdge) && edges.testFlag(Qt::RightEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges.testFlag(Qt::BottomEdge) && edges.testFlag(Qt::LeftEdge)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (edges.testFlag(Qt::BottomEdge) && edges.testFlag(Qt::RightEdge)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (edges.testFlag(Qt::LeftEdge) || edges.testFlag(Qt::RightEdge)) {
        setCursor(Qt::SizeHorCursor);
    } else if (edges.testFlag(Qt::TopEdge) || edges.testFlag(Qt::BottomEdge)) {
        setCursor(Qt::SizeVerCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftPressed = true;
        m_resizeEdges = resizeEdges(event->pos());
        m_resizeStartGeometry = geometry();
        m_resizeStartPos = event->globalPos();
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_leftPressed && m_resizeEdges != Qt::Edges()) {
        performResize(event->globalPos());
        return;
    }
    if (!m_leftPressed) {
        updateResizeCursor(event->pos());
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_leftPressed = false;
    m_resizeEdges = Qt::Edges();
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::leaveEvent(QEvent *event)
{
    setCursor(Qt::ArrowCursor);
    QMainWindow::leaveEvent(event);
}

void MainWindow::performResize(const QPoint &globalPos)
{
    QRect geom = m_resizeStartGeometry;
    const QPoint delta = globalPos - m_resizeStartPos;

    if (m_resizeEdges.testFlag(Qt::LeftEdge)) {
        geom.setLeft(geom.left() + delta.x());
    }
    if (m_resizeEdges.testFlag(Qt::RightEdge)) {
        geom.setRight(geom.right() + delta.x());
    }
    if (m_resizeEdges.testFlag(Qt::TopEdge)) {
        geom.setTop(geom.top() + delta.y());
    }
    if (m_resizeEdges.testFlag(Qt::BottomEdge)) {
        geom.setBottom(geom.bottom() + delta.y());
    }

    if (geom.width() < minimumWidth()) {
        geom.setWidth(minimumWidth());
    }
    if (geom.height() < minimumHeight()) {
        geom.setHeight(minimumHeight());
    }

    setGeometry(geom);
}

void MainWindow::applyTheme(const QString &theme)
{
    setProperty("theme", theme);
    if (_chat) {
        _chat->setProperty("theme", theme);
    }
    qApp->setProperty("theme", theme);

    const bool light = (theme == QStringLiteral("light"));
    const QColor base = light ? QColor(30, 31, 33, 220) : QColor(220, 223, 228, 220);
    const QColor hover = light ? QColor(10, 10, 12, 255) : QColor(255, 255, 255, 230);
    const QColor press = light ? QColor(0, 0, 0, 200) : QColor(255, 255, 255, 200);
    const QString iconPath = light ? QStringLiteral(":/svg/sun.svg") : QStringLiteral(":/svg/moon.svg");

    for (auto *btn : findChildren<AnimatedIconButton*>()) {
        btn->setBaseColor(base);
        btn->setHoverColor(hover);
        btn->setPressColor(press);
        if (btn->objectName() == QStringLiteral("title_theme_btn")) {
            btn->setSvgIcon(iconPath);
        }
    }

    style()->unpolish(this);
    style()->polish(this);
    update();
}
