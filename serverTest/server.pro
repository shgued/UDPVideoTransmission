QT += core
QT -= gui

#header file path
INCLUDEPATH += -I$$"/usr/local/include/opencv2"
#library path
LIBS += -L$$"/usr/local/lib/"

LIBS += -lopencv_core
LIBS += -lopencv_highgui
LIBS += -lopencv_videoio
LIBS += -lopencv_imgcodecs


CONFIG += c++11

TARGET = server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    server.cpp

HEADERS += \
    server.hpp
