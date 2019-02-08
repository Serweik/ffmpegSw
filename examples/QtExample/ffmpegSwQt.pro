#-------------------------------------------------
#
# Project created by QtCreator 2019-01-15T23:48:36
#
#-------------------------------------------------

QT       += core gui
QT		 += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffmpegSwQt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

DEPENDPATH += D:/sourcesLibs/ffmpeg/Windows/include
INCLUDEPATH += D:/sourcesLibs/ffmpeg/Windows/include
win32:LIBS += -LD:/sourcesLibs/ffmpeg/Windows/lib \
         -llibavutil -llibavcodec -llibavdevice -llibavfilter\
         -llibavformat -llibpostproc -llibswresample -llibswscale

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    ../../src/audiodecoder.cpp \
    ../../src/avffmpegwrapper.cpp \
    ../../src/avfilecontext.cpp \
    ../../src/videodecoder.cpp \
    camera.cpp \
    ../../src/avbasedecoder.cpp

HEADERS += \
        mainwindow.h \
    ../../src/audiodecoder.h \
    ../../src/avffmpegwrapper.h \
    ../../src/avfilecontext.h \
    ../../src/avitemcontainer.h \
    ../../src/videodecoder.h \
    camera.h \
    ../../src/avbasedecoder.h

FORMS += \
        mainwindow.ui

CONFIG += mobility
MOBILITY = 


# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target
