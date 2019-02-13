TEMPLATE = app
CONFIG += c++11
CONFIG += console
CONFIG -= c
CONFIG -= qt

win32 {
    DEPENDPATH += D:/sourcesLibs/ffmpeg/Windows/include
    INCLUDEPATH += D:/sourcesLibs/ffmpeg/Windows/include
    LIBS += -LD:/sourcesLibs/ffmpeg/Windows/lib \
             -llibavutil -llibavcodec -llibavdevice -llibavfilter\
             -llibavformat -llibpostproc -llibswresample -llibswscale

    DEPENDPATH += D:/sourcesLibs/SDL2-2.0.9/x86_64-w64-mingw32/include/SDL2
    INCLUDEPATH += D:/sourcesLibs/SDL2-2.0.9/x86_64-w64-mingw32/include/SDL2
    LIBS += -LD:/sourcesLibs/SDL2-2.0.9/x86_64-w64-mingw32/lib -llibSDL2 -llibSDL2main
}

SOURCES += \
        main.cpp \
    ../../src/avffmpegwrapper.cpp \
    ../../src/avfilecontext.cpp \
    ../../src/avbasedecoder.cpp \
    ../../src/videodecoder.cpp \
    ../../src/audiodecoder.cpp

HEADERS += \
    ../../src/avffmpegwrapper.h \
    ../../src/avfilecontext.h \
    ../../src/avbasedecoder.h \
    ../../src/avitemcontainer.h \
    ../../src/videodecoder.h \
    ../../src/audiodecoder.h
