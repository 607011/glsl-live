# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

TARGET = GLSL-Live-Coder
TEMPLATE = app

QT += core gui opengl xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += scripting

# CONFIG += opencv
CONFIG += windowsmediafoundation

TRANSLATIONS = glsl-live_de.ts

CODECFORTR = UTF-8

windowsmediafoundation:!opencv {
    DEFINES += WITH_WINDOWS_MEDIA_FOUNDATION _CRT_SECURE_NO_WARNINGS
    SOURCES += mediainput.cpp mediainputthread.cpp
    HEADERS += mediainput.h mediainputthread.h
    LIBS += ole32.lib oleaut32.lib mfplat.lib mf.lib mfuuid.lib mfreadwrite.lib
}

opencv:!windowsmediafoundation {
    DEFINES += WITH_OPENCV
    SOURCES += mediainput.cpp mediainputthread.cpp
    HEADERS += mediainput.h mediainputthread.h
    win32 {
        DEFINES += _CRT_SECURE_NO_WARNINGS
        LIBS += opencv_core245.lib opencv_highgui245.lib
        QMAKE_CXXFLAGS += -ID:/Developer/opencv/build/include \
            -ID:/Developer/opencv/build/include/opencv \
            -ID:/Developer/opencv/build/include/opencv2
    }
    macx {
        LIBS += -L/opt/local/lib -lopencv_core -lopencv_highgui
        QMAKE_CXXFLAGS += -I/opt/local/include -I/opt/local/include/opencv -I/opt/local/include/opencv2
    }
    unix {
        LIBS += -lopencv_core -lopencv_highgui
    }
}

win32 {
    RC_FILE = glsl-live.rc
}

scripting {
    QT += script
    DEFINES += WITH_SCRIPTING
    SOURCES += editors/js/jsedit.cpp \
        scriptrunner.cpp \
        glclass.cpp \
        imageclass.cpp
    HEADERS += editors/js/jsedit.h \
        scriptrunner.h \
        glclass.h \
        imageclass.h
    OTHER_FILES += defaultscript.js
}

SOURCES += main.cpp\
    mainwindow.cpp \
    renderwidget.cpp \
    renderer.cpp \
    project.cpp \
    editors/glsl/glsledit.cpp \
    editors/glsl/glslhighlighter.cpp \
    editors/glsl/glsldoclayout.cpp \
    editors/sidebarwidget.cpp \
    colorpicker.cpp \
    channelwidget.cpp

HEADERS += main.h \
    mainwindow.h \
    renderwidget.h \
    renderer.h \
    project.h \
    doubleslider.h \
    util.h \
    editors/glsl/glsledit.h \
    editors/glsl/glslhighlighter.h \
    editors/glsl/glsldoclayout.h \
    editors/sidebarwidget.h \
    editors/abstracteditor.h \
    colorpicker.h \
    channelwidget.h

FORMS += mainwindow.ui

RESOURCES += \
    glsl-live.qrc

OTHER_FILES += \
    glsl-live_de.ts \
    LICENSE.txt \
    README.txt \
    doc/index.html \
    examples/default.xml \
    glsl-live.rc
