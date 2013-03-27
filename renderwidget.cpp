// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "renderwidget.h"
#include <QList>
#include <QGLShader>
#include <qmath.h>
#include <QtCore/QDebug>
#include <QMimeData>
#include <QUrl>

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
    , mFirstPaintEventPending(true)
    , mVertexShader(NULL)
    , mFragmentShader(NULL)
    , mShaderProgram(new QGLShaderProgram(this))
    , mTextureHandle(0)
    , mLiveTimerId(0)
{
    mTime.start();
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::MouseFocusReason);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setAcceptDrops(true);
    setMouseTracking(true);
}

RenderWidget::~RenderWidget()
{
    stopCode();
    mShaderProgram->removeAllShaders();
    if (mVertexShader)
        delete mVertexShader;
    if (mFragmentShader)
        delete mFragmentShader;
    delete mShaderProgram;
}

void RenderWidget::setShaderSources(const QString& vs, const QString& fs)
{
    mPreliminaryVertexShaderSource = vs;
    mPreliminaryFragmentShaderSource = fs;
    if (mFirstPaintEventPending)
        return;
    linkProgram(vs, fs);
    if (mShaderProgram->isLinked()) {
        mVertexShaderSource = vs;
        mFragmentShaderSource = fs;
    }
    else {
        // fall back to previous code
        linkProgram(mVertexShaderSource, mFragmentShaderSource);
    }
    if (mShaderProgram->isLinked())
        goLive();
}

void RenderWidget::setImage(const QImage& image)
{
    mImage = image.convertToFormat(QImage::Format_ARGB32);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mImage.width(), mImage.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, mImage.bits());
    if (mShaderProgram->isLinked()) {
        mShaderProgram->setUniformValue(mULocTexture, 0);
        update();
    }
}

bool RenderWidget::linkProgram(const QString& vs, const QString& fs)
{
    bool ok = false;
    if (!vs.isEmpty() && !fs.isEmpty()) {
        mShaderProgram->removeAllShaders();
        if (mVertexShader)
            delete mVertexShader;
        mVertexShader = new QGLShader(QGLShader::Vertex, this);
        ok = mVertexShader->compileSourceCode(vs);
        if (!ok) {
            emit shaderError(mVertexShader->log());
            return false;
        }
        mShaderProgram->addShader(mVertexShader);
        if (mFragmentShader)
            delete mFragmentShader;
        mFragmentShader = new QGLShader(QGLShader::Fragment, this);
        ok = mFragmentShader->compileSourceCode(fs);
        if (!ok) {
            emit shaderError(mFragmentShader->log());
            return false;
        }
        mShaderProgram->addShader(mFragmentShader);
    }
    mShaderProgram->bind();
    mShaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    if (!vs.isEmpty() && !fs.isEmpty()) {
        mShaderProgram->link();
        if (!mShaderProgram->isLinked()) {
            emit shaderError(mShaderProgram->log());
        }
    }
    if (mShaderProgram->isLinked()) {
        mULocT = mShaderProgram->uniformLocation("uT");
        mULocMouse = mShaderProgram->uniformLocation("uMouse");
        mULocResolution = mShaderProgram->uniformLocation("uResolution");
        mULocTexture = mShaderProgram->uniformLocation("uTexture");
        mShaderProgram->setUniformValue(mULocMouse, mMousePos);
        mShaderProgram->setUniformValue(mULocResolution, mResolution);
        mShaderProgram->setUniformValue(mULocTexture, 0);
        emit linkingSuccessful();
        update();
    }
    return ok;
}

void RenderWidget::resizeEvent(QResizeEvent* e)
{
    mResolution = QSizeF(e->size());
    if (mShaderProgram->isLinked())
        mShaderProgram->setUniformValue(mULocResolution, mResolution);
    glViewport(0, 0, e->size().width(), e->size().height());
}

void RenderWidget::goLive()
{
    if (mLiveTimerId == 0)
        mLiveTimerId = startTimer(1000/50);
}

void RenderWidget::stopCode()
{
    if (mLiveTimerId != 0) {
        killTimer(mLiveTimerId);
        mLiveTimerId = 0;
    }
}

void RenderWidget::initializeGL(void)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    linkProgram();
    mShaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    mShaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    mShaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, mVertices);
    mShaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, mTexCoords);

    glGenTextures(1, &mTextureHandle);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mImage.width(), mImage.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, mImage.bits());
}

void RenderWidget::paintGL(void)
{
    if (mFirstPaintEventPending) {
        linkProgram(mPreliminaryVertexShaderSource, mPreliminaryFragmentShaderSource);
        if (mShaderProgram->isLinked())
            goLive();
        mFirstPaintEventPending = false;
    }
    if (mShaderProgram->isLinked()) {
        mShaderProgram->setUniformValue(mULocT, 1e-3f * (GLfloat)mTime.elapsed());
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == mLiveTimerId) {
        update();
    }
    e->accept();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (mShaderProgram->isLinked()) {
        mMousePos = QPointF(e->pos());
        mShaderProgram->setUniformValue(mULocMouse, mMousePos);
        update();
    }
    e->accept();
}

void RenderWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void RenderWidget::dragEnterEvent(QDragEnterEvent* e)
{
    const QMimeData* const d = e->mimeData();
    if (d->hasUrls() && d->urls().first().toString().contains(QRegExp("\\.(png|jpg|gif|ico|mng|tga|tiff?)$")))
        e->acceptProposedAction();
    else
        e->ignore();
}

void RenderWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

void RenderWidget::dropEvent(QDropEvent* e)
{
    const QMimeData* const d = e->mimeData();
    if (d->hasUrls()) {
        QString fileUrl = d->urls().first().toString();
        if (fileUrl.contains(QRegExp("file://.*\\.(png|jpg|gif|ico|mng|tga|tiff?)$")))
#if defined(WIN32)
            loadImage(fileUrl.remove("file:///"));
#else
            loadImage(fileUrl.remove("file://"));
#endif
    }
}

bool RenderWidget::loadImage(const QString& fileName)
{
    if (fileName.isEmpty()) {
        emit shaderError(tr("ImageWidget::loadImage(): fileName is empty."));
        return false;
    }
    QImage image(fileName);
    if (image.isNull())
        return false;
    setImage(image);
    mImageFileName = fileName;
    emit imageDropped(mImage);
    return true;
}

void RenderWidget::setUniformValue(const QString& name, int value)
{
    mShaderProgram->setUniformValue(name.toUtf8().data(), value);
}

void RenderWidget::setUniformValue(const QString& name, float value)
{
    mShaderProgram->setUniformValue(name.toUtf8().data(), value);
}

void RenderWidget::setUniformValue(const QString& name, bool value)
{
    mShaderProgram->setUniformValue(name.toUtf8().data(), value);
}

