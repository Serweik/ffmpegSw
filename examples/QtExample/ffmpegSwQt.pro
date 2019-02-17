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

win32 {
    DEPENDPATH += D:\SourcesLibrerys\ffmpeg/Windows/include
    INCLUDEPATH += D:\SourcesLibrerys\ffmpeg/Windows/include
    LIBS += -LD:\SourcesLibrerys\ffmpeg/Windows/lib \
             -llibavutil -llibavcodec -llibavdevice -llibavfilter\
             -llibavformat -llibpostproc -llibswresample -llibswscale
}

android {
    CONFIG += mobility
    MOBILITY =

    DEFINES += __STDC_CONSTANT_MACROS

    INCLUDEPATH += D:\SourcesLibrerys/ffmpeg/Android/include
    DEPENDPATH += D:\SourcesLibrerys/ffmpeg/Android/include

    ANDROID_EXTRA_LIBS = \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libavcodec.so \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libavdevice.so \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libavfilter.so \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libavformat.so \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libavutil.so \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libswresample.so \
        D:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a/libswscale.so

    LIBS += -LD:\SourcesLibrerys/ffmpeg/Android/lib/armeabi-v7a \
         -lavutil -lavcodec -lavdevice -lavfilter\
         -lavformat -lswresample -lswscale
}


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
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


#DISTFILES += \
#    android/AndroidManifest.xml \
#    android/gradle/wrapper/gradle-wrapper.jar \
#    android/gradlew \
#    android/res/values/libs.xml \
#    android/build.gradle \
#    android/gradle/wrapper/gradle-wrapper.properties \
#    android/gradlew.bat

#contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
#    ANDROID_PACKAGE_SOURCE_DIR = \
#        $$PWD/android

#}
