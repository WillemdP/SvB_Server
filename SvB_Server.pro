#-------------------------------------------------
#
# Project created by QtCreator 2017-08-31T20:36:58
#
#-------------------------------------------------

QT       += core gui network sql concurrent widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SvB_Server
TEMPLATE = app


SOURCES += main.cpp\
        svb_server.cpp \
    tcphost.cpp

HEADERS  += svb_server.h \
    tcphost.h

FORMS    += svb_server.ui
