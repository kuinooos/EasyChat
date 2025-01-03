#ifndef CHAT_DIALOG_H
#define CHAT_DIALOG_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class Chat_Dialog;
}

class Chat_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Chat_Dialog(QWidget *parent = nullptr);
    ~Chat_Dialog();
    void addChatUSerList();

private:
    Ui::Chat_Dialog *ui;
    bool _b_loading;

    QString Send_data;
    QTcpSocket *socket;

    QVector<QString> Other_IP;
private slots:
//    void slot_loading_chat_user();
    void on_chat_edit_textChanged();
    void on_send_btn_clicked();
    void on_readyRead();
    void on_disconnected();
};

#endif // CHAT_DIALOG_H
