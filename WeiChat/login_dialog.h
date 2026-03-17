#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include "server_config.h"

class QLineEdit;
class QLabel;
class QPushButton;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString username() const;
    QString serverHost() const;
    quint16 loginPort() const;
    ServerConfig config() const;

private slots:
    void attemptLogin();

private:
    void applyTheme(const QString &theme);
    quint16 readPort(QLineEdit *edit, quint16 fallback) const;

    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *hostEdit;
    QLineEdit *loginPortEdit;
    QLineEdit *registerPortEdit;
    QLineEdit *chatPortEdit;
    QLineEdit *friendPortEdit;
    QLineEdit *offlinePortEdit;
    QLabel *statusLabel;
    QPushButton *loginButton;
    ServerConfig currentConfig;
};

#endif // LOGIN_DIALOG_H