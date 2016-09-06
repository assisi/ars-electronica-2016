#-------------------------------------------------
#
# Project created by QtCreator 2016-09-02T13:45:46
#
#-------------------------------------------------

QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = assisi-visualizer
TEMPLATE = app


SOURCES += \
    src/main.cpp \
    src/subscriber.cpp \
    src/visualizer.cpp \
    src/msg/base_msgs.pb.cc \
    src/msg/dev_msgs.pb.cc \
    src/msg/sim_msgs.pb.cc

HEADERS  += \
    include/subscriber.h \
    include/visualizer.h \
    include/nzmqt/nzmqt.hpp \
    include/msg/base_msgs.pb.h \
    include/msg/dev_msgs.pb.h \
    include/msg/sim_msgs.pb.h

INCLUDEPATH += \
    include \
    include/msg

FORMS    += \
    ui/vassisi.ui

RESOURCES += \
    resources/artwork.qrc

LIBS += \
    -lzmq \
    -lprotobuf

OTHER_FILES += \
    README.md
