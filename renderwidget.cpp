// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "renderwidget.h"
#include <QtCore/QDebug>
#include <QRegExp>
#include <QList>
#include <QGLShader>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QVector2D>
#include <QVector3D>
#include <QPoint>
#include <QRgb>
#include <QMimeData>
#include <QUrl>
#include <QList>
#include <QPointF>
#include <QTime>
#include <QMap>
#include <QVariant>
#include <QTransform>
#include <qmath.h>

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
        , shaderProgram(new QGLShaderProgram)
        , fbo(NULL)
        , resultImageData(NULL)
        , inputTextureHandle(0)
        , liveTimerId(0)
        , updateImageGL(false)
    { /* ... */ }

    bool firstPaintEventPending;
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QGLFramebufferObject* fbo;
    GLuint* resultImageData;
    GLuint inputTextureHandle;
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
    int uLocMarks;
    int uLocMarksCount;
    bool updateImageGL;
    QMap<QString, QVariant> uniforms;
    QVector<QVector2D> marks;

    void makeShaderProgram(void)
    {
        deleteShaderProgram();
        shaderProgram = new QGLShaderProgram();
        deleteShaders();
        vertexShader = new QGLShader(QGLShader::Vertex);
        fragmentShader = new QGLShader(QGLShader::Fragment);
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
    if (d->firstPaintEventPending)
        return;
    d->preliminaryVertexShaderSource = vs;
    d->preliminaryFragmentShaderSource = fs;
    buildProgram(d->preliminaryVertexShaderSource, d->preliminaryFragmentShaderSource);
    if (d->shaderProgram->isLinked()) {
        d->vertexShaderSource = d->preliminaryVertexShaderSource;
        d->fragmentShaderSource = d->preliminaryFragmentShaderSource;
        emit linkingSuccessful();
    }
    else {
        buildProgram(d->vertexShaderSource, d->fragmentShaderSource);
    }
    if (d->shaderProgram->isLinked()) {
        updateUniforms();
        goLive();
    }
}

void RenderWidget::makeImageFBO(void)
{
    Q_D(RenderWidget);
    makeCurrent();
    if (d->fbo == NULL || d->fbo->size() != d->img.size()) {
        if (d->resultImageData)
            delete [] d->resultImageData;
        d->resultImageData = new GLuint[d->img.width() * d->img.height()];
        if (d->fbo)
            delete d->fbo;
        d->fbo = new QGLFramebufferObject(d->img.size());
    }
}

void RenderWidget::setImage(const QImage& image)
{
    d_ptr->img = image.convertToFormat(QImage::Format_ARGB32);
    d_ptr->updateImageGL = true;
    makeImageFBO();
}

const QString& RenderWidget::imageFileName(void) const
{
     return d_ptr->imgFilename;
}

QImage RenderWidget::resultImage(void)
{
    Q_D(RenderWidget);
    makeCurrent();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, d->img.width(), d->img.height());
    makeImageFBO();
    d->fbo->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glReadPixels(0, 0, d->img.width(), d->img.height(), GL_BGRA, GL_UNSIGNED_BYTE, d->resultImageData);
    d->fbo->release();
    glPopAttrib();
    return QImage(reinterpret_cast<uchar*>(d->resultImageData), d->img.width(), d->img.height(), QImage::Format_RGB32).mirrored(false, true);
}

void RenderWidget::resizeToOriginalImageSize()
{
    resize(d_ptr->img.size());
}

void RenderWidget::updateUniforms()
{
    Q_D(RenderWidget);
    if (!d->shaderProgram->isLinked()) {
        qWarning() << "RenderWidget::updateUniforms() called but shader program not linked";
        return;
    }
    qDebug() << "RenderWidget::updateUniforms()";
    d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
    d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    d->shaderProgram->setUniformValue(d->uLocTexture, 0);
    d->shaderProgram->setUniformValueArray(d->uLocMarks, d->marks.data(), d->marks.size());
    d->shaderProgram->setUniformValue(d->uLocMarksCount, d->marks.size());
    const QList<QString>& keys = d->uniforms.keys();
    for (QList<QString>::const_iterator k = keys.constBegin(); k != keys.constEnd(); ++k) {
        const QString& key = *k;
        const QVariant& value = d->uniforms[key];
        switch (value.type()) {
        case QVariant::Int:
            d->shaderProgram->setUniformValue(key.toUtf8().data(), value.toInt());
            break;
        case QVariant::Double:
            d->shaderProgram->setUniformValue(key.toUtf8().data(), (GLfloat)value.toDouble());
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

void RenderWidget::buildProgram(const QString& vs, const QString& fs)
{
    Q_D(RenderWidget);
    makeCurrent();
    if (vs.isEmpty() || fs.isEmpty())
        return;
    d->makeShaderProgram();
    bool ok;
    ok = d->vertexShader->compileSourceCode(vs);
    if (!ok) {
        emit vertexShaderError(d->vertexShader->log());
        return;
    }
    ok = d->fragmentShader->compileSourceCode(fs);
    if (!ok) {
        emit fragmentShaderError(d->fragmentShader->log());
        return;
    }
    ok = d->shaderProgram->link();
    if (!ok) {
        emit linkerError(d->shaderProgram->log());
        return;
    }
    d->shaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    d->shaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    d->shaderProgram->bind();
    d->shaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    d->shaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    d->shaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, Vertices);
    d->shaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, TexCoords);
    d->uLocT = d->shaderProgram->uniformLocation("uT");
    d->uLocTexture = d->shaderProgram->uniformLocation("uTexture");
    d->uLocMouse = d->shaderProgram->uniformLocation("uMouse");
    d->uLocResolution = d->shaderProgram->uniformLocation("uResolution");
    d->uLocMarks = d->shaderProgram->uniformLocation("uMarks");
    d->uLocMarksCount = d->shaderProgram->uniformLocation("uMarksCount");
}

void RenderWidget::resizeEvent(QResizeEvent* e)
{
    Q_D(RenderWidget);
    d->resolution = QSizeF(e->size());
    if (d->shaderProgram->isLinked()) {
        d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    }
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
    qDebug() << "RenderWidget::initializeGL()";
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glGenTextures(1, &d->inputTextureHandle);
    glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

void RenderWidget::paintGL(void)
{
    Q_D(RenderWidget);
    if (d->firstPaintEventPending) {
        buildProgram(d->preliminaryVertexShaderSource, d->preliminaryFragmentShaderSource);
        if (d->shaderProgram->isLinked())
            goLive();
        d->firstPaintEventPending = false;
    }
    if (d->shaderProgram->isLinked()) {
        d->shaderProgram->setUniformValue(d->uLocT, 1e-3f * (GLfloat)d->time.elapsed());
    }
    if (d->updateImageGL) {
        // Aktualisieren des Bildes *außerhalb* von paintGL() oder initializeGL() ist immer fehlgeschlagen, trotz makeCurrent()
        glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
        d->updateImageGL = false;
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == d_ptr->liveTimerId) {
        updateGL();
    }
    else
        qWarning() << "RenderWidget::timerEven() received bad timer id:" << e->timerId();

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
    switch (e->button()) {
    case Qt::LeftButton:
        d_ptr->marks << QVector2D(double(e->x()) / width(), double(e->y()) / height());
        updateUniforms();
        break;
    case Qt::RightButton:
        d_ptr->marks.clear();
        updateUniforms();
        break;
    default:
        break;
    }
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

