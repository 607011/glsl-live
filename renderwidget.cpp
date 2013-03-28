// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "renderwidget.h"
#include <QList>
#include <QGLShader>
#include <qmath.h>
#include <QtCore/QDebug>
#include <QMimeData>
#include <QUrl>
#include <QList>

static const int PROGRAM_VERTEX_ATTRIBUTE = 0;
static const int PROGRAM_TEXCOORD_ATTRIBUTE = 1;
static const QVector2D TexCoords[4] =
{
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 0),
    QVector2D(1, 1)
};
static const QVector3D Vertices[4] =
{
    QVector2D(-1.0, -1.0),
    QVector2D(-1.0,  1.0),
    QVector2D( 1.0, -1.0),
    QVector2D( 1.0,  1.0)
};


class RenderWidgetPrivate {
public:
    explicit RenderWidgetPrivate(void)
        : firstPaintEventPending(true)
        , vertexShader(NULL)
        , fragmentShader(NULL)
        , fbo(NULL)
        , resultImageData(NULL)
        , inputTextureHandle(0)
        , liveTimerId(0)
    { /* ... */ }

    bool firstPaintEventPending;
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QGLFramebufferObject* fbo;
    GLuint* resultImageData;
    GLuint inputTextureHandle;
    GLenum glerror;
    QTime time;
    QString imgFilename;
    QImage img;
    int liveTimerId;
    QPointF mousePos;
    QSizeF resolution;
    QString preliminaryVertexShaderSource;
    QString preliminaryFragmentShaderSource;
    QString vertexShaderSource;
    QString fragmentShaderSource;
    int uLocT;
    int uLocMouse;
    int uLocResolution;
    int uLocTexture;
    QMap<QString, QVariant> uniforms;

    virtual ~RenderWidgetPrivate()
    {
        if (shaderProgram) {
            shaderProgram->removeAllShaders();
            delete shaderProgram;
        }
        if (fbo)
            delete fbo;
        if (vertexShader)
            delete vertexShader;
        if (fragmentShader)
            delete fragmentShader;
        if (resultImageData)
            delete [] resultImageData;
    }
};

RenderWidget::RenderWidget(QWidget* parent)
    : QGLWidget(parent)
    , d_ptr(new RenderWidgetPrivate)
{
    d_ptr->shaderProgram = new QGLShaderProgram(this);
    d_ptr->time.start();
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::MouseFocusReason);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setAcceptDrops(true);
    setMouseTracking(true);
}

RenderWidget::~RenderWidget()
{
    stopCode();
}

void RenderWidget::setShaderSources(const QString& vs, const QString& fs)
{
    d_ptr->preliminaryVertexShaderSource = vs;
    d_ptr->preliminaryFragmentShaderSource = fs;
    if (d_ptr->firstPaintEventPending)
        return;
    linkProgram(vs, fs);
    if (d_ptr->shaderProgram->isLinked()) {
        d_ptr->vertexShaderSource = vs;
        d_ptr->fragmentShaderSource = fs;
    }
    else {
        // fall back to previous code
        linkProgram(d_ptr->vertexShaderSource, d_ptr->fragmentShaderSource);
    }
    if (d_ptr->shaderProgram->isLinked())
        goLive();
}

void RenderWidget::makeImageFBO(void)
{
    if (context()->isValid()) {
        if (d_ptr->fbo == NULL || d_ptr->fbo->size() != d_ptr->img.size()) {
            if (d_ptr->fbo)
                qDebug() << d_ptr->fbo->size() << d_ptr->img.size();
            if (d_ptr->resultImageData)
                delete [] d_ptr->resultImageData;
            d_ptr->resultImageData = new GLuint[d_ptr->img.width() * d_ptr->img.height()];
            if (d_ptr->fbo)
                delete d_ptr->fbo;
            d_ptr->fbo = new QGLFramebufferObject(d_ptr->img.size());
            qDebug() << d_ptr->fbo->size();
        }
    }
}

void RenderWidget::setImage(const QImage& image)
{
    d_ptr->img = image.convertToFormat(QImage::Format_ARGB32);
    glBindTexture(GL_TEXTURE_2D, d_ptr->inputTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_ptr->img.width(), d_ptr->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d_ptr->img.bits());
    if (d_ptr->shaderProgram->isLinked()) {
        d_ptr->shaderProgram->setUniformValue(d_ptr->uLocTexture, (GLuint)0);
        update();
    }
    makeImageFBO();
}

const QString& RenderWidget::imageFileName(void) const
{
     return d_ptr->imgFilename;
}

QImage RenderWidget::resultImage(void)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, d_ptr->img.width(), d_ptr->img.height());
    makeImageFBO();
    d_ptr->fbo->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glReadPixels(0, 0, d_ptr->img.width(), d_ptr->img.height(), GL_BGRA, GL_UNSIGNED_BYTE, d_ptr->resultImageData);
    d_ptr->fbo->release();
    glPopAttrib();
    return QImage(reinterpret_cast<uchar*>(d_ptr->resultImageData), d_ptr->img.width(), d_ptr->img.height(), QImage::Format_RGB32);
}

void RenderWidget::resizeToOriginalImageSize()
{
    resize(d_ptr->img.size());
}

void RenderWidget::updateUniforms()
{
    if (!d_ptr->shaderProgram->isLinked())
        return;
    d_ptr->shaderProgram->setUniformValue(d_ptr->uLocMouse, d_ptr->mousePos);
    d_ptr->shaderProgram->setUniformValue(d_ptr->uLocResolution, d_ptr->resolution);
    d_ptr->shaderProgram->setUniformValue(d_ptr->uLocTexture, 0);
    const QList<QString>& keys = d_ptr->uniforms.keys();
    for (QList<QString>::const_iterator k = keys.constBegin(); k != keys.constEnd(); ++k) {
        const QString& key = *k;
        const QVariant& value = d_ptr->uniforms[key];
        switch (value.type()) {
        case QVariant::Int:
            d_ptr->shaderProgram->setUniformValue(key.toUtf8().data(), value.toInt());
            break;
        case QVariant::Double:
            d_ptr->shaderProgram->setUniformValue(key.toUtf8().data(), (float)value.toDouble());
            break;
        case QVariant::Bool:
            d_ptr->shaderProgram->setUniformValue(key.toUtf8().data(), value.toBool());
            break;
        default:
            qWarning() << "RenderWidget::updateUniforms(): invalid value type in mUniforms";
            break;
        }
    }
    update();
}

void RenderWidget::clearUniforms(void)
{
    d_ptr->uniforms.clear();
}

bool RenderWidget::linkProgram(const QString& vs, const QString& fs)
{
    bool ok = false;
    if (!vs.isEmpty() && !fs.isEmpty()) {
        d_ptr->shaderProgram->release();
        d_ptr->shaderProgram->removeAllShaders();
        if (d_ptr->vertexShader)
            delete d_ptr->vertexShader;
        d_ptr->vertexShader = new QGLShader(QGLShader::Vertex, this);
        ok = d_ptr->vertexShader->compileSourceCode(vs);
        if (!ok) {
            emit shaderError(d_ptr->vertexShader->log());
            return false;
        }
        d_ptr->shaderProgram->addShader(d_ptr->vertexShader);
        if (d_ptr->fragmentShader)
            delete d_ptr->fragmentShader;
        d_ptr->fragmentShader = new QGLShader(QGLShader::Fragment, this);
        ok = d_ptr->fragmentShader->compileSourceCode(fs);
        if (!ok) {
            emit shaderError(d_ptr->fragmentShader->log());
            return false;
        }
        d_ptr->shaderProgram->addShader(d_ptr->fragmentShader);
    }
    d_ptr->shaderProgram->bind();
    d_ptr->shaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    d_ptr->shaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    if (!vs.isEmpty() && !fs.isEmpty()) {
        d_ptr->shaderProgram->link();
        if (!d_ptr->shaderProgram->isLinked()) {
            emit shaderError(d_ptr->shaderProgram->log());
        }
    }
    if (d_ptr->shaderProgram->isLinked()) {
        d_ptr->uLocT = d_ptr->shaderProgram->uniformLocation("uT");
        d_ptr->uLocMouse = d_ptr->shaderProgram->uniformLocation("uMouse");
        d_ptr->uLocResolution = d_ptr->shaderProgram->uniformLocation("uResolution");
        d_ptr->uLocTexture = d_ptr->shaderProgram->uniformLocation("uTexture");
        updateUniforms();
        emit linkingSuccessful();
        update();
    }
    return ok;
}

void RenderWidget::resizeEvent(QResizeEvent* e)
{
    d_ptr->resolution = QSizeF(e->size());
    if (d_ptr->shaderProgram->isLinked())
        d_ptr->shaderProgram->setUniformValue(d_ptr->uLocResolution, d_ptr->resolution);
    glViewport(0, 0, e->size().width(), e->size().height());
}

void RenderWidget::goLive()
{
    if (d_ptr->liveTimerId == 0)
        d_ptr->liveTimerId = startTimer(1000/50);
}

void RenderWidget::stopCode()
{
    if (d_ptr->liveTimerId != 0) {
        killTimer(d_ptr->liveTimerId);
        d_ptr->liveTimerId = 0;
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
    d_ptr->shaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    d_ptr->shaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    d_ptr->shaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, Vertices);
    d_ptr->shaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, TexCoords);

    glGenTextures(1, &d_ptr->inputTextureHandle);
    glBindTexture(GL_TEXTURE_2D, d_ptr->inputTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_ptr->img.width(), d_ptr->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d_ptr->img.bits());
}

void RenderWidget::paintGL(void)
{
    if (d_ptr->firstPaintEventPending) {
        linkProgram(d_ptr->preliminaryVertexShaderSource, d_ptr->preliminaryFragmentShaderSource);
        if (d_ptr->shaderProgram->isLinked())
            goLive();
        d_ptr->firstPaintEventPending = false;
    }
    if (d_ptr->shaderProgram->isLinked()) {
        d_ptr->shaderProgram->setUniformValue(d_ptr->uLocT, 1e-3f * (GLfloat)d_ptr->time.elapsed());
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == d_ptr->liveTimerId) {
        update();
    }
    e->accept();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (d_ptr->shaderProgram->isLinked()) {
        d_ptr->mousePos = QPointF(e->pos());
        d_ptr->shaderProgram->setUniformValue(d_ptr->uLocMouse, d_ptr->mousePos);
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
    d_ptr->imgFilename = fileName;
    emit imageDropped(d_ptr->img);
    return true;
}

const QImage &RenderWidget::image(void) const
{
    return d_ptr->img;
}

void RenderWidget::setUniformValue(const QString& name, int value)
{
    d_ptr->uniforms[name] = value;
    updateUniforms();
}

void RenderWidget::setUniformValue(const QString& name, double value)
{
    d_ptr->uniforms[name] = value;
    updateUniforms();
}

void RenderWidget::setUniformValue(const QString& name, bool value)
{
    d_ptr->uniforms[name] = value;
    updateUniforms();
}

