#ifndef CLICKBTN_H
#define CLICKBTN_H
#include<QPushButton>
#include<QMouseEvent>
class ClickBtn:public QPushButton
{
    Q_OBJECT
public:
    ClickBtn(QWidget*parent=nullptr);
    ~ClickBtn();
    void SetState(QString nomal,QString hover,QString press);
protected:
    virtual void enterEvent(QEvent *event)override;//鼠标进入
    virtual void leaveEvent(QEvent *event)override;
    virtual void mousePressEvent(QMouseEvent *event)override;//鼠标按下
    virtual void mouseReleaseEvent(QMouseEvent *event)override;//鼠标释放
private:
    QString _normal;
    QString _hover;
    QString _press;//三态的属性表示

};

#endif // CLICKBTN_H
