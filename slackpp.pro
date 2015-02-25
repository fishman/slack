#-------------------------------------------------
#
# Project created by QtCreator 2015-02-23T17:40:28
#
#-------------------------------------------------

TEMPLATE = app
TARGET = slack

CONFIG += -std=gnu++11
QT       += webenginewidgets network widgets dbus webchannel gui


qtHaveModule(uitools):!embedded: QT += uitools
else: DEFINES += QT_NO_UITOOLS

SOURCES += main.cpp\
        mainwindow.cpp\
        browserapplication.cpp\
        webview.cpp

HEADERS  += mainwindow.h\
    browserapplication.h\
    webview.h \

RESOURCES += data/data.qrc

FORMS    += mainwindow.ui
