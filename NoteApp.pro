QT += core gui webenginewidgets webchannel widgets

CONFIG += c++17
CONFIG -= qtquickcompiler

LIBS += -luser32

SOURCES += \
    document.cpp \
    link_dialog.cpp \
    main.cpp \
    noteapp.cpp \
    previewpage.cpp

HEADERS += \
    document.h \
    link_dialog.h \
    noteapp.h \
    previewpage.h

FORMS += \
    link_dialog.ui \
    noteapp.ui

RESOURCES = \
    icons/icons.qrc \
    resources/package.qrc

RESOURCES += qdarkstyle/dark/style.qrc

RC_ICONS = icons/NoteApp.ico
