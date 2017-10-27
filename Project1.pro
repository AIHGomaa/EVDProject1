#-------------------------------------------------
#
# Project created by QtCreator 2017-10-05T17:58:20
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Project1
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    sevensegmentgaugereader.cpp \
    measureunit.cpp \
    readerresult.cpp \
    imageanalizer.cpp \
    digitfeatures.cpp

HEADERS += \
        mainwindow.h \
    sevensegmentgaugereader.h \
    measureunit.h \
    readerresult.h \
    igaugereader.h \
    imageanalizer.h \
    digitfeatures.h

FORMS += \
        mainwindow.ui

INCLUDEPATH += "c:\opencv-3.3\opencv\build\install\include"

LIBS += "c:\opencv-3.3\opencv\build\install\x86\mingw\bin\libopencv*.dll"
