QT += widgets serialport multimedia
TARGET = tpo_info2_qt
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    audio_sender.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    wav.h \
    audio_sender.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

