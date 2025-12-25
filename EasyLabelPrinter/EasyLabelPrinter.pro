QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EasyLabelPrinter
#程序图标
RC_ICONS = EasyLabelPrinter.ico

#程序版本
VERSION = 2025.07.20


QMAKE_TARGET_COMPANY = "Home Office: Levi.Wei"
#产品名称
QMAKE_TARGET_PRODUCT = EasyLabelPrinter
#版权所有
QMAKE_TARGET_COPYRIGHT = "微信: 13537556177"
#文件说明
QMAKE_TARGET_DESCRIPTION = "EasyLabelPrinter By QT6.8.0"

RC_LANG = 0x0004


CONFIG += c++17

LIBS += -L$$PWD -lzint

DESTDIR = ../../bin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CLabelSave.cpp \
    CustomItems.cpp \
    FrameLabelView.cpp \
    FramePrintControl.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    CLabelSave.h \
    CustomItems.h \
    FrameLabelView.h \
    FramePrintControl.h \
    mainwindow.h

FORMS += \
    FrameLabelView.ui \
    FramePrintControl.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
