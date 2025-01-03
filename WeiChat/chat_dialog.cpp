#include "chat_dialog.h"
#include "ui_chat_dialog.h"
#include<QRandomGenerator>

#include<QAction>
#include<QTextEdit>
#include<QTcpSocket>
#include<chatuserwid.h>
Chat_Dialog::Chat_Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Chat_Dialog)
{
    ui->setupUi(this);
    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));
    // 创建一个清除动作并设置图标
    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);
    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 文本为空时，切换回透明图标
        }
    });
    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 清除文本后，切换回透明图标
        ui->search_edit->clearFocus();
        //清除按钮被按下则不显示搜索框
        //ShowSearch(false);
    });
    ui->search_edit->SetMaxLength(15);

//    connect(ui->chat_user_list,&ChatUserList::sig_loading_chat_user,this,&Chat_Dialog::slot_loading_chat_user);//gaidon

    addChatUSerList();
}

Chat_Dialog::~Chat_Dialog()
{
    if (socket->isOpen()) {
            socket->disconnectFromHost();
        }
    delete socket;
    delete ui;
}

std::vector<QString>  strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                             "You have to love yourself",
                             "My love is written in the wind ever since the whole world is you"};

std::vector<QString> heads = {
    ":/png/xyy.jpg",
    ":/png/fyy.jpg",
    ":/png/myy.jpg",
    ":/png/lyy.jpg",
    ":/png/htl.jpg",
    ":/png/xsg.jpg",
    ":/png/hds.jpg"
};

std::vector<QString> names = {
    "喜洋洋",
    "沸羊羊",
    "美羊羊",
    "懒羊羊",
    "灰太狼",
    "潇洒哥",
    "黑大帅"
};//测试




void Chat_Dialog::addChatUSerList()
{
    // 创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue%strs.size();
        int head_i = randomValue%heads.size();
        int name_i = randomValue%names.size();

        auto *chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
    }
}

void Chat_Dialog::on_chat_edit_textChanged()
{
    QString currentText = ui->chat_edit->toPlainText();

    qDebug() << "Current Text:" << currentText;

    Send_data = currentText;
}

void Chat_Dialog::on_send_btn_clicked()
{
    if (socket->isOpen() && !Send_data.isEmpty()) {
           QByteArray data = Send_data.toUtf8();
           socket->write(data + '\n');  // 添加换行符便于服务器解析
           socket->flush();
       }
}

void Chat_Dialog::on_readyRead()
{
    while (socket->canReadLine()) {
        QString message = socket->readLine().trimmed();
        ui->listWidget->addItem("Server: " + message);
    }
}

void Chat_Dialog::on_disconnected()
{
    qDebug() << "Disconnected from server!";
    ui->listWidget->addItem("Disconnected from server!");
}
