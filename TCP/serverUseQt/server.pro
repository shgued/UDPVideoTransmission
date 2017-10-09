#-------------------------------------------------
#
# Project created by QtCreator 2017-08-07T18:59:05
#
#-------------------------------------------------

QT       += core gui network

#header file path
INCLUDEPATH += -I$$"/usr/local/include/opencv2"
#library path
LIBS += -L$$"/usr/local/lib/"

LIBS += -lopencv_core
LIBS += -lopencv_highgui
LIBS += -lopencv_videoio
LIBS += -lopencv_imgcodecs


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    server.cpp

HEADERS  += mainwindow.h \
    server.hpp

FORMS    += mainwindow.ui

SUBDIRS += \
