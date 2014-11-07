QT += widgets serialport multimedia
TARGET = tpo_info2_qt
TEMPLATE = app
CONFIG += c++11


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    audio_sender.cpp \
    client.cpp \
    protocol.c

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    wav.h \
    audio_sender.h \
    protocol.h \
    client.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

