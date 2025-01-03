QT += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
RC_ICONS='weichat.ico'
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    chat_dialog.cpp \
    chatuserlist.cpp \
    chatuserwid.cpp \
    customizeedit.cpp \
    global.cpp \
    listitembase.cpp \
    loadingdlg.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    chat_dialog.h \
    chatuserlist.h \
    chatuserwid.h \
    customizeedit.h \
    global.h \
    listitembase.h \
    loadingdlg.h \
    mainwindow.h

FORMS += \
    chat_dialog.ui \
    chatuserwid.ui \
    loadingdlg.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rs.qrc

DISTFILES +=
