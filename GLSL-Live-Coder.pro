# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT += core gui opengl xml

CONFIG += scripting
# CONFIG += multimedia
CONFIG += opencv

opencv {
DEFINES += WITH_OPENCV
LIBS += -L$${OPENCVDIR}/lib -lopencv_core240 -lopencv_highgui240
QMAKE_CXXFLAGS += -ID:\Developer\opencv\include\opencv -ID:\Developer\opencv\include
# set LIB and INCLUDE accordingly in Qt Creator's build configuration, PATH in run configuration
}

multimedia {
QT += multimedia
DEFINES += ENABLE_CAMERA ENABLE_SOUND
}

TARGET = GLSL-Live-Coder
TEMPLATE = app

win32 {
RC_FILE = glsl-live.rc
}

scripting {
QT += script
DEFINES += ENABLE_SCRIPTING
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
