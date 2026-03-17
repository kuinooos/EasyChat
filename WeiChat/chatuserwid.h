#ifndef CHATUSERWID_H
#define CHATUSERWID_H
#include <QWidget>
#include "listitembase.h"
namespace Ui {
class ChatUserWid;
}
class ChatUserWid : public ListItemBase
{
    Q_OBJECT
public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();
    QSize sizeHint() const override {
        return QSize(280, 78);
    }
    void SetInfo(const QString &name, const QString &head, const QString &preview, bool online);
    void setOnlineState(bool online, const QString &preview);
    void setSelected(bool selected);
    QString userName() const;
private:
    Ui::ChatUserWid *ui;
    QString _name;
    QString _head;
    QString _preview;
    bool _online = false;
};
#endif // CHATUSERWID_H
