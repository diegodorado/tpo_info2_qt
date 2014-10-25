QT += widgets serialport multimedia
TARGET = tpo_info2_qt
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    audio_sender.cpp \
    audio_buffer.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    wav.h \
    audio_sender.h \
    audio_buffer.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

