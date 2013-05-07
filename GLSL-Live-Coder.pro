# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT += core gui opengl xml

CONFIG += videocapture scripting

videocapture:win32 {
DEFINES += _CRT_SECURE_NO_WARNINGS
SOURCES += webcamthread.cpp
HEADERS += webcamthread.h
LIBS += ole32.lib oleaut32.lib mfplat.lib mf.lib mfplay.lib mfuuid.lib mfreadwrite.lib
}

opencv {
DEFINES += WITH_OPENCV
win32 {
DEFINES += _CRT_SECURE_NO_WARNINGS
LIBS += -lopencv_core245 -lopencv_highgui245
}
macx {
LIBS += -L/opt/local/lib -lopencv_core -lopencv_highgui
}
SOURCES += webcamthread.cpp
HEADERS += webcamthread.h
}

TARGET = GLSL-Live-Coder
TEMPLATE = app

win32 {
RC_FILE = glsl-live.rc
}

macx {
QMAKE_CXXFLAGS += -I/opt/local/include -I/opt/local/include/opencv -I/opt/local/include/opencv2
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

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TRANSLATIONS = glsl-live_de.ts

CODECFORTR = UTF-8


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
    channelwidget.cpp \
    videocapturedevice.cpp

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
    channelwidget.h \
    videocapturedevice.h

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
