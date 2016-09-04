#-------------------------------------------------
#
# Project created by QtCreator 2016-09-02T13:45:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = assisi-visualizer
TEMPLATE = app


SOURCES += main.cpp \
    visualizer.cpp \
    subscriber.cpp

HEADERS  += \
    visualizer.h \
    subscriber.h \
    nzmqt/nzmqt.hpp \
    nzmqt/impl.hpp

FORMS    += \
    vassisi.ui

RESOURCES += \
    artwork.qrc

LIBS += \
    -lzmq
