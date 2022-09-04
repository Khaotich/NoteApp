QT += core gui webenginewidgets webchannel widgets sql

CONFIG += c++17
CONFIG -= qtquickcompiler

LIBS += -luser32

SOURCES += \
    document.cpp \
    link_dialog.cpp \
    main.cpp \
    noteapp.cpp \
    photo_dialog.cpp \
    previewpage.cpp

HEADERS += \
    document.h \
    link_dialog.h \
    noteapp.h \
    photo_dialog.h \
    previewpage.h

FORMS += \
    link_dialog.ui \
    noteapp.ui \
    photo_dialog.ui

RESOURCES = \
    icons/icons.qrc \
    resources/package.qrc

RESOURCES += qdarkstyle/dark/style.qrc

RC_ICONS = icons/NoteApp.ico
