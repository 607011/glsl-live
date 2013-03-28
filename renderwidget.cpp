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
        , shaderProgram(NULL)
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

    void makeShaderProgram(QObject* parent = NULL)
    {
        deleteShaderProgram();
        shaderProgram = new QGLShaderProgram(parent);
    }

    void makeShaders(QObject* parent = NULL)
    {
        deleteShaders();
        vertexShader = new QGLShader(QGLShader::Vertex, parent);
        fragmentShader = new QGLShader(QGLShader::Fragment, parent);
        if (shaderProgram) {
            shaderProgram->addShader(vertexShader);
            shaderProgram->addShader(fragmentShader);
        }
    }

    virtual ~RenderWidgetPrivate()
    {
        deleteShaderProgram();
        deleteShaders();
        if (fbo)
            delete fbo;
        if (resultImageData)
            delete [] resultImageData;
    }

private:
    void deleteShaders()
    {
        if (vertexShader) {
            delete vertexShader;
            vertexShader = NULL;
        }
        if (fragmentShader) {
            delete fragmentShader;
            fragmentShader = NULL;
        }
    }

    void deleteShaderProgram(void)
    {
        if (shaderProgram) {
            shaderProgram->removeAllShaders();
            delete shaderProgram;
            shaderProgram = NULL;
        }
    }
};

RenderWidget::RenderWidget(QWidget* parent)
    : QGLWidget(parent)
    , d_ptr(new RenderWidgetPrivate)
{
    d_ptr->makeShaderProgram(this);
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
    Q_D(RenderWidget);
    d->preliminaryVertexShaderSource = vs;
    d->preliminaryFragmentShaderSource = fs;
    if (d->firstPaintEventPending)
        return;
    linkProgram(vs, fs);
    if (d->shaderProgram->isLinked()) {
        d->vertexShaderSource = vs;
        d->fragmentShaderSource = fs;
    }
    else {
        // fall back to previous code
        linkProgram(d->vertexShaderSource, d->fragmentShaderSource);
    }
    if (d->shaderProgram->isLinked())
        goLive();
}

void RenderWidget::makeImageFBO(void)
{
    Q_D(RenderWidget);
    if (context()->isValid()) {
        if (d->fbo == NULL || d->fbo->size() != d->img.size()) {
            if (d->resultImageData)
                delete [] d->resultImageData;
            d->resultImageData = new GLuint[d->img.width() * d->img.height()];
            if (d->fbo)
                delete d->fbo;
            d->fbo = new QGLFramebufferObject(d->img.size());
        }
    }
}

void RenderWidget::setImage(const QImage& image)
{
    Q_D(RenderWidget);
    d->img = image.convertToFormat(QImage::Format_ARGB32);
    glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
    if (d->shaderProgram->isLinked()) {
        d->shaderProgram->setUniformValue(d->uLocTexture, (GLuint)0);
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
    Q_D(RenderWidget);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, d->img.width(), d->img.height());
    makeImageFBO();
    d->fbo->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glReadPixels(0, 0, d->img.width(), d->img.height(), GL_BGRA, GL_UNSIGNED_BYTE, d->resultImageData);
    d->fbo->release();
    glPopAttrib();
    return QImage(reinterpret_cast<uchar*>(d->resultImageData), d->img.width(), d->img.height(), QImage::Format_RGB32);
}

void RenderWidget::resizeToOriginalImageSize()
{
    resize(d_ptr->img.size());
}

void RenderWidget::updateUniforms()
{
    Q_D(RenderWidget);
    if (!d->shaderProgram->isLinked())
        return;
    d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
    d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    d->shaderProgram->setUniformValue(d->uLocTexture, 0);
    const QList<QString>& keys = d->uniforms.keys();
    for (QList<QString>::const_iterator k = keys.constBegin(); k != keys.constEnd(); ++k) {
        const QString& key = *k;
        const QVariant& value = d->uniforms[key];
        switch (value.type()) {
        case QVariant::Int:
            d->shaderProgram->setUniformValue(key.toUtf8().data(), value.toInt());
            break;
        case QVariant::Double:
            d->shaderProgram->setUniformValue(key.toUtf8().data(), (float)value.toDouble());
            break;
        case QVariant::Bool:
            d->shaderProgram->setUniformValue(key.toUtf8().data(), value.toBool());
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
    Q_D(RenderWidget);
    bool ok = false;
    if (!vs.isEmpty() && !fs.isEmpty()) {
        d->makeShaderProgram();
        d->makeShaders();
        ok = d->vertexShader->compileSourceCode(vs);
        if (!ok) {
            emit shaderError(d->vertexShader->log());
            return false;
        }
        ok = d->fragmentShader->compileSourceCode(fs);
        if (!ok) {
            emit shaderError(d->fragmentShader->log());
            return false;
        }
    }
    d->shaderProgram->bind();
    d->shaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    d->shaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    if (!vs.isEmpty() && !fs.isEmpty()) {
        d->shaderProgram->link();
        if (!d->shaderProgram->isLinked()) {
            emit shaderError(d->shaderProgram->log());
        }
    }
    if (d->shaderProgram->isLinked()) {
        d->uLocT = d->shaderProgram->uniformLocation("uT");
        d->uLocMouse = d->shaderProgram->uniformLocation("uMouse");
        d->uLocResolution = d->shaderProgram->uniformLocation("uResolution");
        d->uLocTexture = d->shaderProgram->uniformLocation("uTexture");
        updateUniforms();
        emit linkingSuccessful();
        update();
    }
    return ok;
}

void RenderWidget::resizeEvent(QResizeEvent* e)
{
    Q_D(RenderWidget);
    d->resolution = QSizeF(e->size());
    if (d->shaderProgram->isLinked())
        d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
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
    Q_D(RenderWidget);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    linkProgram();
    d->shaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    d->shaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    d->shaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, Vertices);
    d->shaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, TexCoords);

    glGenTextures(1, &d->inputTextureHandle);
    glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
}

void RenderWidget::paintGL(void)
{
    Q_D(RenderWidget);
    if (d->firstPaintEventPending) {
        linkProgram(d->preliminaryVertexShaderSource, d->preliminaryFragmentShaderSource);
        if (d->shaderProgram->isLinked())
            goLive();
        d->firstPaintEventPending = false;
    }
    if (d->shaderProgram->isLinked()) {
        d->shaderProgram->setUniformValue(d->uLocT, 1e-3f * (GLfloat)d->time.elapsed());
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == d_ptr->liveTimerId) {
        updateGL();
    }
    e->accept();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    Q_D(RenderWidget);
    if (d->shaderProgram->isLinked()) {
        d->mousePos = QPointF(e->pos());
        d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
        updateGL();
    }
    e->accept();
}

void RenderWidget::mousePressEvent(QMouseEvent* e)
{
    QSizeF pos = QSizeF(double(e->x()) / width(), double(e->y()) / height());
    qDebug() << pos;
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
    if (fileName.isEmpty())
        return false;
    QImage image(fileName);
    if (image.isNull())
        return false;
    setImage(image);
    d_ptr->imgFilename = fileName;
    emit imageDropped(d_ptr->img);
    return true;
}

const QImage& RenderWidget::image(void) const
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

