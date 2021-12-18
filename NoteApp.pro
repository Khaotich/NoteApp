QT += core gui webenginewidgets webchannel

QT += widgets

CONFIG += c++17
CONFIG -= qtquickcompiler

LIBS += -luser32

SOURCES += \
    document.cpp \
    main.cpp \
    noteapp.cpp \
    previewpage.cpp

HEADERS += \
    document.h \
    noteapp.h \
    previewpage.h

FORMS += \
    noteapp.ui

RESOURCES = \
    icons/icons.qrc \
    resources/package.qrc

RESOURCES += qdarkstyle/dark/style.qrc
RESOURCES += icons/icons.qrc
