QT += core
QT -= gui

#head file path
INCLUDEPATH += -I$$"/usr/local/include/opencv2"
#lib path
LIBS += -L$$"/usr/local/lib/"

LIBS += -lopencv_core
LIBS += -lopencv_highgui
LIBS += -lopencv_videoio
LIBS += -lopencv_imgcodecs


CONFIG += c++11

TARGET = client
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    client.cpp

HEADERS  += \
    client.hpp
