// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "renderwidget.h"
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QGLShader>
#include <qmath.h>
#include <QtCore/QDebug>

static const int PROGRAM_VERTEX_ATTRIBUTE = 0;
static const int PROGRAM_TEXCOORD_ATTRIBUTE = 1;

const QVector2D RenderWidget::mTexCoords[4] = {
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 0),
    QVector2D(1, 1)
};


const QVector3D RenderWidget::mVertices[4] = {
    QVector2D(-1.0, -1.0),
    QVector2D(-1.0,  1.0),
    QVector2D( 1.0, -1.0),
    QVector2D( 1.0,  1.0)
};


RenderWidget::RenderWidget(QWidget* parent)
    : QGLWidget(parent)
    , mShaderProgram(new QGLShaderProgram(this))
    , mTextureHandle(0)
    , mImage(QImage(":/images/passbild.jpg"))
    , mTimerId(0)
{
    mTime.start();
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::MouseFocusReason);
    setMouseTracking(true);
}

RenderWidget::~RenderWidget()
{
    stopCode();
    delete mShaderProgram;
}

void RenderWidget::setShaderSources(const QString& vs, const QString& fs)
{
    mShaderProgram->addShaderFromSourceCode(QGLShader::Vertex, vs);
    mShaderProgram->addShaderFromSourceCode(QGLShader::Fragment, fs);
    mShaderProgram->link();
    if (mShaderProgram->isLinked()) {
        qDebug() << "Linking Shader Program failed: " << mShaderProgram->log();
        stopCode();
    }
    else {
        setupShaderProgram();
        goLive();
    }
}

void RenderWidget::goLive()
{
    stopCode();
    mTimerId = startTimer(1000/60);
}

void RenderWidget::stopCode()
{
    if (mTimerId) {
        killTimer(mTimerId);
        mTimerId = 0;
    }
}

void RenderWidget::setupShaderProgram()
{
    mShaderProgram->bind();
    mShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);
    mShaderProgram->setUniformValue("uTexture", 0);
}

void RenderWidget::initializeGL(void)
{
    qDebug() << "RenderWidget::initializeGL()";
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);

    mShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);

    glGenTextures(1, &mTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mImage.width(), mImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, mImage.bits());
}


void RenderWidget::paintGL(void)
{
    if (mShaderProgram->isLinked()) {
        mShaderProgram->setUniformValue("uT", 1e-3f * (GLfloat)mTime.elapsed());
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == mTimerId) {
        updateGL();
    }
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (mShaderProgram->isLinked()) {
        mShaderProgram->setUniformValue("uMouse", QPointF(e->pos()));
    }
}

void RenderWidget::resizeGL(int width, int height)
{
    if (mShaderProgram->isLinked()) {
        mShaderProgram->setUniformValue("uWindowSize", size());
    }
    glViewport(0, 0, width, height);
}
