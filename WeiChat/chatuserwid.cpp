#include "chatuserwid.h"
#include "ui_chatuserwid.h"
#include <QStyle>
ChatUserWid::ChatUserWid(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ChatUserWid)
{
    ui->setupUi(this);
    setObjectName("chat_user_item");
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_Hover, true);
    SetItemType(ListItemType::CHAT_USER_ITEM);
}
ChatUserWid::~ChatUserWid()
{
    delete ui;
}
void ChatUserWid::SetInfo(const QString &name, const QString &head, const QString &preview, bool online)
{
    _name = name;
    _head = head;
    _preview = preview;
    _online = online;
    // 加载图片
    QPixmap pixmap(_head);
    // 设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);
    ui->user_name_lb->setText(_name);
    setOnlineState(_online, _preview);
}

void ChatUserWid::setOnlineState(bool online, const QString &preview)
{
    _online = online;
    _preview = preview;
    setProperty("online", online);

    ui->user_chat_lb->setText(_preview);
    ui->user_status_lb->setText(online ? QStringLiteral("在线") : QStringLiteral("离线"));
    ui->status_wid->setProperty("online", online);

    style()->unpolish(this);
    style()->polish(this);
    style()->unpolish(ui->status_wid);
    style()->polish(ui->status_wid);
    update();
}

void ChatUserWid::setSelected(bool selected)
{
    setProperty("selected", selected);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

QString ChatUserWid::userName() const
{
    return _name;
}
