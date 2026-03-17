#include "mainwindow.h"
#include "login_dialog.h"

#include <QApplication>
#include <QFont>
#include<QFile>
int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);

    QFont appFont(QStringLiteral("Microsoft YaHei UI"), 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    a.setFont(appFont);

    //用qss对界面进行修饰
    QFile qss(":/style/stylesheet.qss");
    if( qss.open(QFile::ReadOnly))
    {
        qDebug("open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
        qDebug("Open failed");
    }
    //用qss对界面进行修饰

    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow w(loginDialog.username(), loginDialog.config());
    w.show();
    return a.exec();
}
