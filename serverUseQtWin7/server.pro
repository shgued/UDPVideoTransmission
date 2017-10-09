#-------------------------------------------------
#
# Project created by QtCreator 2017-08-07T18:59:05
#
#-------------------------------------------------

QT       += core gui network

##header file path
#INCLUDEPATH += -I$$"/usr/local/include/opencv2"
##library path
#LIBS += -L$$"/usr/local/lib/"

#LIBS += -lopencv_core
#LIBS += -lopencv_highgui
#LIBS += -lopencv_videoio
#LIBS += -lopencv_imgcodecs

#header file path
DEST = "Program Files"
INCLUDEPATH += "D:/Program Files/openCV3/build/include"

#library path
DEST = "Program Files"
LIBS += "D:/Program Files/openCV3/lib/libopencv_core320.dll.a"
LIBS += "D:/Program Files/openCV3/lib/libopencv_highgui320.dll.a"
LIBS += "D:/Program Files/openCV3/lib/libopencv_videoio320.dll.a"
LIBS += "D:/Program Files/openCV3/lib/libopencv_imgcodecs320.dll.a"

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
