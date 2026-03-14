#ifndef GLOBAL_H
#define GLOBAL_H
#include<QWidget>


enum ListItemType{
    CHAT_USER_ITEM,//聊天用户
    CONTACT_USER_ITEM,//联系人用户
    SEARCH_USER_ITEM,//搜索到的用户
    ADD_USER_TIP_ITEM,//提示添加用户
    INVALID_ITEM,//不可点击
    GROUP_TIP_ITEM,//分组提示
};

class global
{
public:
    global();
};

#endif // GLOBAL_H
