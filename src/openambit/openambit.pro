#-------------------------------------------------
#
# Project created by QtCreator 2013-11-24T20:10:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = openambit
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    devicemanager.cpp \
    settingsdialog.cpp \
    settings.cpp \
    logstore.cpp \
    logentry.cpp \
    movescount.cpp

HEADERS  += mainwindow.h \
    devicemanager.h \
    settingsdialog.h \
    settings.h \
    logstore.h \
    logentry.h \
    movescount.h

FORMS    += mainwindow.ui \
    settingsdialog.ui

INCLUDEPATH += ../libambit
QMAKE_LIBDIR += ../build/libambit

LIBS += -lambit
