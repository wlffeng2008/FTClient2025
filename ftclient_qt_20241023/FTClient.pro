QT += core gui network printsupport mqtt  multimedia bluetooth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FTClient
#程序图标
RC_ICONS = FTClient.ico

#程序版本
VERSION = 2025.12.25

#产品名称
QMAKE_TARGET_PRODUCT = FTClient
#版权所有
QMAKE_TARGET_COPYRIGHT = TS@Video Tech.
#文件说明
QMAKE_TARGET_DESCRIPTION = Factory Test Client

DESTDIR = $$PWD/bin

CONFIG += c++17
CONFIG += lrelease

LIBS += -lws2_32
LIBS += -L$$PWD -lzint

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/../ffmpeg-7.0.2/include
LIBS += $$PWD/../ffmpeg-7.0.2/lib/*.lib

INCLUDEPATH += ./Main \
               ./LabelTools \
               ./Http \
               ./Tcp \
               ./CommonWgts \
               ./SysSettings \
               ./BatchTest \
               ./VideoAndPtz \
               ./DefaultValuesWidget \
               ./UI \
               ./Log

SOURCES += \
    BatchTest/BatchTestWidget.cpp \
    BatchTest/dialogbluetooth.cpp \
    DefaultValuesWidget/DialogDefaultValues.cpp \
    Http/HttpForTange.cpp \
    LabelTools/CLabelEdit.cpp \
    LabelTools/CLabelSave.cpp \
    LabelTools/CLabelSettings.cpp \
    LabelTools/CustomItems.cpp \
    LabelTools/Label.cpp \
    LabelTools/WgtLabelParaStart.cpp \
    Main/CommonLib.cpp \
    Main/MainWidget.cpp\
    Main/dialogboxlabelprint.cpp \
    Http/CHttpClient.cpp \
    Http/CHttpClientAgent.cpp \
    Log/CLog.cpp \
    Main/dialogpacklabelprint.cpp \
    SysSettings/CSysSettings.cpp \
    SysSettings/DialogSysSetting.cpp \
    UI/uiGlobal.cpp \
    VideoAndPtz/CRtmpVideoHandle.cpp \
    VideoAndPtz/VideoAndPtzWidget.cpp \
    VideoAndPtz/caudioplayer.cpp \
    main.cpp

HEADERS += \
    BatchTest/BatchTestWidget.h \
    BatchTest/dialogbluetooth.h \
    DefaultValuesWidget/DialogDefaultValues.h \
    Http/HttpForTange.h \
    LabelTools/CLabelEdit.h \
    LabelTools/CLabelSave.h \
    LabelTools/CLabelSettings.h \
    LabelTools/CustomItems.h \
    LabelTools/Label.h \
    LabelTools/WgtLabelParaStart.h \
    Main/CommonLib.h \
    Main/dialogboxlabelprint.h \
    CommonWgts/ButtonDelegateManPass.h \
    Http/CHttpClient.h \
    Http/CHttpClientAgent.h \
    Log/CLog.h \
    Main/MainWidget.h \
    Main/dialogpacklabelprint.h \
    SysSettings/CSysSettings.h \
    SysSettings/DialogSysSetting.h \
    CommonWgts/ButtonDelegate.h \
    UI/uiGlobal.h \
    VideoAndPtz/CRtmpVideoHandle.h \
    VideoAndPtz/VideoAndPtzWidget.h \
    VideoAndPtz/caudioplayer.h

FORMS += \
    BatchTest/BatchTestWidget.ui \
    BatchTest/dialogbluetooth.ui \
    DefaultValuesWidget/DialogDefaultValues.ui \
    LabelTools/CLabelSettings.ui \
    LabelTools/WgtLabelParaStart.ui \
    Main/MainWidget.ui \
    Main/dialogboxlabelprint.ui \
    Main/dialogpacklabelprint.ui \
    SysSettings/DialogSysSetting.ui \
    VideoAndPtz/VideoAndPtzWidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
