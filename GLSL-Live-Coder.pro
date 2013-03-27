# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT       += core gui opengl xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GLSL-Live-Coder
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    renderwidget.cpp \
    jsedit/jsedit.cpp \
    project.cpp

HEADERS  += mainwindow.h \
    renderwidget.h \
    jsedit/jsedit.h \
    main.h \
    project.h \
    doubleslider.h

FORMS    += mainwindow.ui

RESOURCES += \
    shaders.qrc \
    images.qrc

OTHER_FILES += \
    shaders/fragmentshader.glsl \
    shaders/vertexshader.glsl \
    LICENSE.txt \
    examples/crosshatch.xml \
    TODO.txt
