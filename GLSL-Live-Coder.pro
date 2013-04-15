# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT       += core gui opengl xml script

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TRANSLATIONS = glsl-live_de.ts

CODECFORTR = UTF-8

TARGET = GLSL-Live-Coder
TEMPLATE = app

SOURCES += main.cpp\
    mainwindow.cpp \
    renderwidget.cpp \
    renderer.cpp \
    project.cpp \
    editors/glsl/glsledit.cpp \
    editors/glsl/glslhighlighter.cpp \
    editors/glsl/glsldoclayout.cpp \
    editors/js/jsedit.cpp \
    editors/sidebarwidget.cpp \
    scriptrunner.cpp \
    glclass.cpp

HEADERS  += main.h \
    mainwindow.h \
    renderwidget.h \
    renderer.h \
    project.h \
    doubleslider.h \
    util.h \
    editors/glsl/glsledit.h \
    editors/glsl/glslhighlighter.h \
    editors/glsl/glsldoclayout.h \
    editors/js/jsedit.h \
    editors/sidebarwidget.h \
    editors/abstracteditor.h \
    scriptrunner.h \
    glclass.h

FORMS += mainwindow.ui

RESOURCES += \
    shaders.qrc \
    images.qrc \
    help.qrc \
    translations.qrc

OTHER_FILES += \
    glsl-live_de.ts \
    LICENSE.txt \
    doc/index.html \
    shaders/fragmentshader.glsl \
    shaders/vertexshader.glsl \
    examples/crosshatch.xml \
    examples/gaussian.xml \
    examples/toad-torch.xml \
    examples/nyan-ct-banner.xml \
    examples/game-of-life.xml \
    examples/wobble-and-zoom.xml \
    examples/warholizer.xml \
    examples/spotlight-by-rasmuskaae.xml \
    examples/skip.xml \
    examples/scanline-effect.xml \
    examples/rgb-separation-grey.xml \
    examples/rgb-separation.xml \
    examples/glitch2.xml \
    examples/fractalmorph.xml \
    examples/aberration.xml \
    defaultscript.js
