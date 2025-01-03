#include "mainwindow.h"
#include "ui_mainwindow.h"
#include"chat_dialog.h"
#include<QObject>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _chat=new Chat_Dialog(this);
    setCentralWidget(_chat);
    _chat->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    _chat->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}
