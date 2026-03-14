#ifndef CHAT_DIALOG_H
#define CHAT_DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include "server_config.h"

class QPushButton;
class QListWidgetItem;
class QCloseEvent;

namespace Ui {
class Chat_Dialog;
}

class Chat_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Chat_Dialog(const QString &username, const ServerConfig &serverConfig, QWidget *parent = nullptr);
    ~Chat_Dialog();
    void addChatUSerList();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Chat_Dialog *ui;
    bool _b_loading;

    QString username;
    ServerConfig serverConfig;
    QString currentPeer;
    QString Send_data;
    QTcpSocket *socket;
    bool offlineNotified = false;

    QVector<QString> Other_IP;
    void initializeConnection();
    void loadFriendList();
    void addChatUserItem(const QString &name, const QString &head, const QString &msg, bool selectByDefault = false);
    QString sendFriendCommand(const QString &command);
    void updateCurrentPeer(const QString &peerName);
    void notifyOffline();
private slots:
//    void slot_loading_chat_user();
    void on_chat_edit_textChanged();
    void on_send_btn_clicked();
    void on_readyRead();
    void on_disconnected();
    void onChatUserItemClicked(QListWidgetItem *item);
    void onAddFriendClicked();
};

#endif // CHAT_DIALOG_H
