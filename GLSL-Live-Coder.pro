# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

QT       += core gui opengl xml concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TRANSLATIONS = glsl-live_de.ts

CODECFORTR = UTF-8

TARGET = GLSL-Live-Coder
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    renderwidget.cpp \
    glsledit/glsledit.cpp \
    project.cpp \
    glsledit/glslhighlighter.cpp \
    glsledit/sidebarwidget.cpp \
    glsledit/glsldoclayout.cpp \
    renderer.cpp

HEADERS  += mainwindow.h \
    renderwidget.h \
    glsledit/glsledit.h \
    main.h \
    project.h \
    doubleslider.h \
    glsledit/glslhighlighter.h \
    glsledit/sidebarwidget.h \
    glsledit/glsldoclayout.h \
    util.h \
    renderer.h

FORMS    += mainwindow.ui

RESOURCES += \
    shaders.qrc \
    images.qrc \
    help.qrc \
    translations.qrc

OTHER_FILES += \
    shaders/fragmentshader.glsl \
    shaders/vertexshader.glsl \
    LICENSE.txt \
    examples/crosshatch.xml \
    doc/index.html \
    examples/chromatic.xml \
    examples/ontoeidetik.xml \
    examples/gaussian.xml \
    examples/toad-torch.xml \
    glsl-live_de.ts \
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
    examples/glitch.xml \
    examples/fractalmorph.xml \
    examples/aberration.xml
