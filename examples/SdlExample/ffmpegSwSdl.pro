TEMPLATE = app
CONFIG += c++17
CONFIG += console
CONFIG -= c
CONFIG -= qt

DEPENDPATH += D:/SourcesLibrerys/ffmpeg-4.1-win64-dev/include
INCLUDEPATH += D:/SourcesLibrerys/ffmpeg-4.1-win64-dev/include
win32:LIBS += -LD:/SourcesLibrerys/ffmpeg-4.1-win64-dev/lib \
         -llibavutil -llibavcodec -llibavdevice -llibavfilter\
         -llibavformat -llibpostproc -llibswresample -llibswscale


DEPENDPATH += D:/SourcesLibrerys/SDL2-2.0.9/x86_64-w64-mingw32/include/SDL2
INCLUDEPATH += D:/SourcesLibrerys/SDL2-2.0.9/x86_64-w64-mingw32/include/SDL2
win32:LIBS += -LD:/SourcesLibrerys/SDL2-2.0.9/x86_64-w64-mingw32/lib -llibSDL2 -llibSDL2main

SOURCES += \
        main.cpp \
    ../../src/avffmpegwrapper.cpp \
    ../../src/avfilecontext.cpp \
    ../../src/avabstractdecoder.cpp \
    ../../src/videodecoder.cpp \
    ../../src/audiodecoder.cpp

HEADERS += \
    ../../src/avffmpegwrapper.h \
    ../../src/avfilecontext.h \
    ../../src/avabstactdecoder.h \
    ../../src/avitemcontainer.h \
    ../../src/videodecoder.h \
    ../../src/audiodecoder.h
