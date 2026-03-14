#include "clickbtn.h"
#include<QMouseEvent>
#include<QVariant>
#include"global.h"

ClickBtn::ClickBtn(QWidget *parent):QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
}


ClickBtn::~ClickBtn(){

}

void ClickBtn::SetState(QString normal,QString hover,QString press){
    _hover=hover;
    _normal=normal;
    _press=press;
    setProperty("state",normal);
    repolish(this);
    update();
}

void ClickBtn::enterEvent(QEvent *event){
    setProperty("state",_hover);
    repolish(this);
    update();
    QPushButton::enterEvent(event);
}


void ClickBtn::mousePressEvent(QMouseEvent *event){
    setProperty("state",_press);
    repolish(this);
    update();
    QPushButton::mousePressEvent(event);
}


void ClickBtn::mouseReleaseEvent(QMouseEvent *event){
    setProperty("state",_hover);
    repolish(this);
    update();
    QPushButton::mouseReleaseEvent(event);
}
