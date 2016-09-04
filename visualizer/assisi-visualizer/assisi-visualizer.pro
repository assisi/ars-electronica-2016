#-------------------------------------------------
#
# Project created by QtCreator 2016-09-02T13:45:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = assisi-visualizer
TEMPLATE = app


SOURCES += \
    src/main.cpp \
    src/subscriber.cpp \
    src/visualizer.cpp

HEADERS  += \
    include/subscriber.h \
    include/visualizer.h \
    include/nzmqt/nzmqt.hpp

FORMS    += \
    ui/vassisi.ui

RESOURCES += \
    resources/artwork.qrc

LIBS += \
    -lzmq
