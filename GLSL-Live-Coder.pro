
QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GLSL-Live-Coder
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    renderwidget.cpp \
    jsedit/jsedit.cpp

HEADERS  += mainwindow.h \
    renderwidget.h \
    jsedit/jsedit.h \
    main.h

FORMS    += mainwindow.ui

RESOURCES += \
    shaders.qrc \
    images.qrc

OTHER_FILES += \
    shaders/fragmentshader.glsl \
    shaders/vertexshader.glsl \
    Passbild-Oliver-Lau.jpg
