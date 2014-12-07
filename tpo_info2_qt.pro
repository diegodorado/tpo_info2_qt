QT += widgets serialport multimedia
TARGET = tpo_info2_qt
TEMPLATE = app
CONFIG += c++11


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    client.cpp \
    protocol.c

HEADERS += \
    mainwindow.h \
    protocol.h \
    client.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    icons.qrc

