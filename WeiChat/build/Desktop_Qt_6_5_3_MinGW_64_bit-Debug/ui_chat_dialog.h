/********************************************************************************
** Form generated from reading UI file 'chat_dialog.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHAT_DIALOG_H
#define UI_CHAT_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <chatuserlist.h>
#include <customizeedit.h>

QT_BEGIN_NAMESPACE

class Ui_Chat_Dialog
{
public:
    QHBoxLayout *horizontalLayout;
    QWidget *side_bar;
    QVBoxLayout *verticalLayout;
    QWidget *widget_2;
    QVBoxLayout *verticalLayout_3;
    QLabel *side_connect_lb;
    QLabel *side_head_lb;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *side_chat_lb;
    QSpacerItem *verticalSpacer;
    QWidget *widget_3;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_4;
    QLabel *label_6;
    QLabel *label_5;
    QWidget *chat_user_wid;
    QVBoxLayout *verticalLayout_2;
    QWidget *search_wid;
    QHBoxLayout *horizontalLayout_2;
    CustomizeEdit *search_edit;
    QSpacerItem *horizontalSpacer;
    QPushButton *pushButton;
    ChatUserList *chat_user_list;
    QWidget *chat_data_wid;
    QVBoxLayout *verticalLayout_4;
    QWidget *title_wid;
    QVBoxLayout *verticalLayout_5;
    QWidget *widget;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label;
    QLabel *label_7;
    QWidget *chat_data_list;
    QWidget *funct_wid;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_2;
    QLabel *emo_lb;
    QSpacerItem *horizontalSpacer_4;
    QLabel *file_lb;
    QSpacerItem *horizontalSpacer_8;
    QLabel *label_8;
    QSpacerItem *horizontalSpacer_7;
    QLabel *label_9;
    QSpacerItem *horizontalSpacer_3;
    QTextEdit *chat_edit;
    QWidget *send_wid;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *send_btn;
    QSpacerItem *horizontalSpacer_6;

    void setupUi(QDialog *Chat_Dialog)
    {
        if (Chat_Dialog->objectName().isEmpty())
            Chat_Dialog->setObjectName("Chat_Dialog");
        Chat_Dialog->resize(1138, 799);
        Chat_Dialog->setMinimumSize(QSize(1138, 799));
        Chat_Dialog->setMaximumSize(QSize(1138, 799));
        horizontalLayout = new QHBoxLayout(Chat_Dialog);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        side_bar = new QWidget(Chat_Dialog);
        side_bar->setObjectName("side_bar");
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(side_bar->sizePolicy().hasHeightForWidth());
        side_bar->setSizePolicy(sizePolicy);
        side_bar->setMinimumSize(QSize(70, 0));
        side_bar->setMaximumSize(QSize(70, 16777215));
        verticalLayout = new QVBoxLayout(side_bar);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(10, 30, 0, 20);
        widget_2 = new QWidget(side_bar);
        widget_2->setObjectName("widget_2");
        widget_2->setMinimumSize(QSize(0, 30));
        verticalLayout_3 = new QVBoxLayout(widget_2);
        verticalLayout_3->setSpacing(30);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        side_connect_lb = new QLabel(widget_2);
        side_connect_lb->setObjectName("side_connect_lb");
        side_connect_lb->setMinimumSize(QSize(44, 44));
        side_connect_lb->setMaximumSize(QSize(44, 44));
        side_connect_lb->setPixmap(QPixmap(QString::fromUtf8(":/png/user_img.jpg")));

        verticalLayout_3->addWidget(side_connect_lb);

        side_head_lb = new QLabel(widget_2);
        side_head_lb->setObjectName("side_head_lb");
        side_head_lb->setMinimumSize(QSize(44, 44));
        side_head_lb->setMaximumSize(QSize(44, 44));
        side_head_lb->setPixmap(QPixmap(QString::fromUtf8(":/png/left_1.png")));

        verticalLayout_3->addWidget(side_head_lb);

        label_2 = new QLabel(widget_2);
        label_2->setObjectName("label_2");
        label_2->setMinimumSize(QSize(44, 44));
        label_2->setMaximumSize(QSize(44, 44));
        label_2->setPixmap(QPixmap(QString::fromUtf8(":/png/user.png")));

        verticalLayout_3->addWidget(label_2);

        label_3 = new QLabel(widget_2);
        label_3->setObjectName("label_3");
        label_3->setMinimumSize(QSize(44, 44));
        label_3->setMaximumSize(QSize(44, 44));
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/png/left_3.png")));

        verticalLayout_3->addWidget(label_3);

        side_chat_lb = new QLabel(widget_2);
        side_chat_lb->setObjectName("side_chat_lb");
        side_chat_lb->setMinimumSize(QSize(44, 44));
        side_chat_lb->setMaximumSize(QSize(44, 44));
        side_chat_lb->setPixmap(QPixmap(QString::fromUtf8(":/png/friend.png")));

        verticalLayout_3->addWidget(side_chat_lb);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);


        verticalLayout->addWidget(widget_2);

        widget_3 = new QWidget(side_bar);
        widget_3->setObjectName("widget_3");
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(widget_3->sizePolicy().hasHeightForWidth());
        widget_3->setSizePolicy(sizePolicy1);
        widget_3->setMinimumSize(QSize(0, 80));
        widget_3->setMaximumSize(QSize(16777215, 16777215));
        verticalLayout_6 = new QVBoxLayout(widget_3);
        verticalLayout_6->setSpacing(30);
        verticalLayout_6->setObjectName("verticalLayout_6");
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        label_4 = new QLabel(widget_3);
        label_4->setObjectName("label_4");
        label_4->setMinimumSize(QSize(44, 44));
        label_4->setMaximumSize(QSize(44, 44));
        label_4->setPixmap(QPixmap(QString::fromUtf8(":/png/left_bt1.png")));

        verticalLayout_6->addWidget(label_4);

        label_6 = new QLabel(widget_3);
        label_6->setObjectName("label_6");
        label_6->setMinimumSize(QSize(44, 44));
        label_6->setMaximumSize(QSize(44, 44));
        label_6->setPixmap(QPixmap(QString::fromUtf8(":/png/phone.png")));

        verticalLayout_6->addWidget(label_6);

        label_5 = new QLabel(widget_3);
        label_5->setObjectName("label_5");
        label_5->setMinimumSize(QSize(44, 44));
        label_5->setMaximumSize(QSize(44, 44));
        label_5->setPixmap(QPixmap(QString::fromUtf8(":/png/other.png")));

        verticalLayout_6->addWidget(label_5);


        verticalLayout->addWidget(widget_3);


        horizontalLayout->addWidget(side_bar);

        chat_user_wid = new QWidget(Chat_Dialog);
        chat_user_wid->setObjectName("chat_user_wid");
        chat_user_wid->setMinimumSize(QSize(313, 0));
        chat_user_wid->setMaximumSize(QSize(313, 16777215));
        verticalLayout_2 = new QVBoxLayout(chat_user_wid);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        search_wid = new QWidget(chat_user_wid);
        search_wid->setObjectName("search_wid");
        search_wid->setMinimumSize(QSize(0, 60));
        search_wid->setMaximumSize(QSize(16777215, 60));
        horizontalLayout_2 = new QHBoxLayout(search_wid);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        search_edit = new CustomizeEdit(search_wid);
        search_edit->setObjectName("search_edit");
        search_edit->setMinimumSize(QSize(0, 30));
        search_edit->setMaximumSize(QSize(16777215, 30));

        horizontalLayout_2->addWidget(search_edit);

        horizontalSpacer = new QSpacerItem(5, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        pushButton = new QPushButton(search_wid);
        pushButton->setObjectName("pushButton");
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(pushButton->sizePolicy().hasHeightForWidth());
        pushButton->setSizePolicy(sizePolicy2);
        pushButton->setMinimumSize(QSize(30, 30));
        pushButton->setMaximumSize(QSize(30, 30));
        pushButton->setStyleSheet(QString::fromUtf8("QPushButton{border-image:url(:/png/grey_209.png)}\n"
"QPushButton:hover{border-image: url(:/png/grey_226.png)}\n"
"QPushButton:pressed{border-image: url(:/png/grey_209.png)}"));

        horizontalLayout_2->addWidget(pushButton);


        verticalLayout_2->addWidget(search_wid);

        chat_user_list = new ChatUserList(chat_user_wid);
        chat_user_list->setObjectName("chat_user_list");

        verticalLayout_2->addWidget(chat_user_list);


        horizontalLayout->addWidget(chat_user_wid);

        chat_data_wid = new QWidget(Chat_Dialog);
        chat_data_wid->setObjectName("chat_data_wid");
        verticalLayout_4 = new QVBoxLayout(chat_data_wid);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        title_wid = new QWidget(chat_data_wid);
        title_wid->setObjectName("title_wid");
        title_wid->setMinimumSize(QSize(0, 60));
        title_wid->setMaximumSize(QSize(16777215, 60));
        verticalLayout_5 = new QVBoxLayout(title_wid);
        verticalLayout_5->setObjectName("verticalLayout_5");
        widget = new QWidget(title_wid);
        widget->setObjectName("widget");
        horizontalLayout_5 = new QHBoxLayout(widget);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        label = new QLabel(widget);
        label->setObjectName("label");

        horizontalLayout_5->addWidget(label);

        label_7 = new QLabel(widget);
        label_7->setObjectName("label_7");
        label_7->setMinimumSize(QSize(30, 30));
        label_7->setMaximumSize(QSize(30, 30));
        label_7->setPixmap(QPixmap(QString::fromUtf8(":/png/more.png")));

        horizontalLayout_5->addWidget(label_7);


        verticalLayout_5->addWidget(widget);


        verticalLayout_4->addWidget(title_wid);

        chat_data_list = new QWidget(chat_data_wid);
        chat_data_list->setObjectName("chat_data_list");

        verticalLayout_4->addWidget(chat_data_list);

        funct_wid = new QWidget(chat_data_wid);
        funct_wid->setObjectName("funct_wid");
        funct_wid->setMinimumSize(QSize(0, 50));
        funct_wid->setMaximumSize(QSize(16777215, 50));
        horizontalLayout_3 = new QHBoxLayout(funct_wid);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(2, 2, 2, 2);
        horizontalSpacer_2 = new QSpacerItem(5, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        emo_lb = new QLabel(funct_wid);
        emo_lb->setObjectName("emo_lb");
        emo_lb->setMinimumSize(QSize(25, 25));
        emo_lb->setMaximumSize(QSize(25, 25));
        emo_lb->setPixmap(QPixmap(QString::fromUtf8(":/png/expression.png")));

        horizontalLayout_3->addWidget(emo_lb);

        horizontalSpacer_4 = new QSpacerItem(5, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_4);

        file_lb = new QLabel(funct_wid);
        file_lb->setObjectName("file_lb");
        file_lb->setMinimumSize(QSize(25, 25));
        file_lb->setMaximumSize(QSize(25, 25));
        file_lb->setPixmap(QPixmap(QString::fromUtf8(":/png/ltwj.png")));

        horizontalLayout_3->addWidget(file_lb);

        horizontalSpacer_8 = new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_8);

        label_8 = new QLabel(funct_wid);
        label_8->setObjectName("label_8");
        label_8->setMinimumSize(QSize(25, 25));
        label_8->setMaximumSize(QSize(25, 25));
        label_8->setPixmap(QPixmap(QString::fromUtf8(":/png/jietu.png")));

        horizontalLayout_3->addWidget(label_8);

        horizontalSpacer_7 = new QSpacerItem(5, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_7);

        label_9 = new QLabel(funct_wid);
        label_9->setObjectName("label_9");
        label_9->setMinimumSize(QSize(25, 25));
        label_9->setMaximumSize(QSize(25, 25));
        label_9->setPixmap(QPixmap(QString::fromUtf8(":/png/record.png")));

        horizontalLayout_3->addWidget(label_9);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        verticalLayout_4->addWidget(funct_wid);

        chat_edit = new QTextEdit(chat_data_wid);
        chat_edit->setObjectName("chat_edit");
        chat_edit->setMinimumSize(QSize(0, 150));
        chat_edit->setMaximumSize(QSize(16777215, 160));

        verticalLayout_4->addWidget(chat_edit);

        send_wid = new QWidget(chat_data_wid);
        send_wid->setObjectName("send_wid");
        send_wid->setMinimumSize(QSize(0, 100));
        send_wid->setMaximumSize(QSize(16777215, 100));
        horizontalLayout_4 = new QHBoxLayout(send_wid);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_5);

        send_btn = new QPushButton(send_wid);
        send_btn->setObjectName("send_btn");
        send_btn->setMinimumSize(QSize(100, 30));
        send_btn->setMaximumSize(QSize(100, 30));

        horizontalLayout_4->addWidget(send_btn);

        horizontalSpacer_6 = new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_6);


        verticalLayout_4->addWidget(send_wid);


        horizontalLayout->addWidget(chat_data_wid);


        retranslateUi(Chat_Dialog);

        QMetaObject::connectSlotsByName(Chat_Dialog);
    } // setupUi

    void retranslateUi(QDialog *Chat_Dialog)
    {
        Chat_Dialog->setWindowTitle(QCoreApplication::translate("Chat_Dialog", "Dialog", nullptr));
        side_connect_lb->setText(QString());
        side_head_lb->setText(QString());
        label_2->setText(QString());
        label_3->setText(QString());
        side_chat_lb->setText(QString());
        label_4->setText(QString());
        label_6->setText(QString());
        label_5->setText(QString());
        pushButton->setText(QString());
        label->setText(QCoreApplication::translate("Chat_Dialog", "\350\213\246\347\223\234\345\244\247\347\216\213", nullptr));
        label_7->setText(QString());
        emo_lb->setText(QString());
        file_lb->setText(QString());
        label_8->setText(QString());
        label_9->setText(QString());
        send_btn->setText(QCoreApplication::translate("Chat_Dialog", "\345\217\221\351\200\201(S)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Chat_Dialog: public Ui_Chat_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHAT_DIALOG_H
