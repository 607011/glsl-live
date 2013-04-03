# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT       += core gui opengl xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GLSL-Live-Coder
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    renderwidget.cpp \
    glsledit/glsledit.cpp \
    project.cpp \
    glsledit/glslhighlighter.cpp \
    glsledit/sidebarwidget.cpp \
    glsledit/glsldoclayout.cpp

HEADERS  += mainwindow.h \
    renderwidget.h \
    glsledit/glsledit.h \
    main.h \
    project.h \
    doubleslider.h \
    glsledit/glslhighlighter.h \
    glsledit/sidebarwidget.h \
    glsledit/glsldoclayout.h \
    util.h

FORMS    += mainwindow.ui

RESOURCES += \
    shaders.qrc \
    images.qrc \
    help.qrc

OTHER_FILES += \
    shaders/fragmentshader.glsl \
    shaders/vertexshader.glsl \
    LICENSE.txt \
    examples/crosshatch.xml \
    TODO.txt \
    doc/index.html \
    examples/chromatic.xml \
    examples/ontoeidetik.xml \
    examples/gaussian.xml
