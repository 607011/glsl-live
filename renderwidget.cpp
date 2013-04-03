// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QRegExp>
#include <QStringList>
#include <QGLShader>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QVector2D>
#include <QVector3D>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRgb>
#include <QMimeData>
#include <QUrl>
#include <QTime>
#include <QMap>
#include <QVariant>
#include <qmath.h>
#include "renderwidget.h"
#include "util.h"

static const int PROGRAM_VERTEX_ATTRIBUTE = 0;
static const int PROGRAM_TEXCOORD_ATTRIBUTE = 1;
static const QVector2D TexCoords[4] =
{
    QVector2D(0, 0),
    QVector2D(0, 1),
    QVector2D(1, 0),
    QVector2D(1, 1)
};
static const QVector2D Vertices[4] =
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
        , scale(1.0)
        , leftMouseButtonPressed(false)
        , mouseMovedWhileLeftButtonPressed(false)
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
    double scale;
    QPoint offset;
    bool leftMouseButtonPressed;
    bool mouseMovedWhileLeftButtonPressed;
    QPoint lastMousePos;
    QRect viewport;
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
        safeDelete(fbo);
        safeDeleteArray(resultImageData);
    }

private:
    void deleteShaders()
    {
        safeDelete(vertexShader);
        safeDelete(fragmentShader);
    }

    void deleteShaderProgram(void)
    {
        if (shaderProgram) {
            shaderProgram->removeAllShaders();
            safeDelete(shaderProgram);
        }
    }
};

RenderWidget::RenderWidget(QWidget* parent)
    : QGLWidget(parent)
    , d_ptr(new RenderWidgetPrivate)
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::MouseFocusReason);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setAcceptDrops(true);
    setMouseTracking(true);
    d_ptr->time.start();
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
        safeDeleteArray(d->resultImageData);
        d->resultImageData = new GLuint[d->img.width() * d->img.height()];
        safeDelete(d->fbo);
        d->fbo = new QGLFramebufferObject(d->img.size());
    }
}

void RenderWidget::setImage(const QImage& img)
{
    Q_D(RenderWidget);
    d->img = img.convertToFormat(QImage::Format_ARGB32);
    d->updateImageGL = true;
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

void RenderWidget::updateUniforms()
{
    Q_D(RenderWidget);
    if (!d->shaderProgram->isLinked())
        return;
    d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
    d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    d->shaderProgram->setUniformValue(d->uLocTexture, 0);
    d->shaderProgram->setUniformValueArray(d->uLocMarks, d->marks.data(), d->marks.size());
    d->shaderProgram->setUniformValue(d->uLocMarksCount, d->marks.size());
    QStringListIterator k(d->uniforms.keys());
    while (k.hasNext()) {
        const QString& key = k.next();
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

void RenderWidget::calcViewport(const QSize& size)
{
    calcViewport(size.width(), size.height());
}

void RenderWidget::calcViewport(int w, int h)
{
    Q_D(RenderWidget);
    const QSizeF& glSize = d->scale * QSizeF(d->img.size());
    const QPoint& topLeft = QPoint(w - glSize.width(), h - glSize.height()) / 2;
    d->viewport = QRect(topLeft + d->offset, glSize.toSize());
    glViewport(d->viewport.x(), d->viewport.y(), d->viewport.width(), d->viewport.height());
    d->resolution = QSizeF(d->viewport.size());
    if (d->shaderProgram->isLinked())
        d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    updateGL();
}

void RenderWidget::resizeEvent(QResizeEvent* e)
{
    calcViewport(e->size());
}

void RenderWidget::fitImageToWindow(void)
{
    Q_D(RenderWidget);
    const double imageAspectRatio = double(d->img.width()) / d->img.height();
    const double windowAspectRatio = double(width()) / height();
    d->scale = (imageAspectRatio > windowAspectRatio)
            ? double(width()) / d->img.width()
            : double(height()) / d->img.height();
    d->offset = QPoint(0, 0);
    calcViewport(size());
}

void RenderWidget::resizeToOriginalImageSize(void)
{
    Q_D(RenderWidget);
    d->scale = 1.0;
    d->offset = QPoint(0, 0);
    calcViewport(size());
}

void RenderWidget::resizeGL(int w, int h)
{
    calcViewport(w, h);
    updateGL();
}

void RenderWidget::initializeGL(void)
{
    Q_D(RenderWidget);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
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
    glClear(GL_COLOR_BUFFER_BIT);
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
        d->updateImageGL = false;
        // Aktualisierung hier, weil Aktualisieren des Bildes
        // *außerhalb* von paintGL() oder initializeGL() trotz
        // makeCurrent() immer fehlgeschlagen ist
        glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == d_ptr->liveTimerId)
        update();
    else
        qWarning() << "RenderWidget::timerEvent() received bad timer id:" << e->timerId();
    e->accept();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    Q_D(RenderWidget);
    if (d->leftMouseButtonPressed) {
        d->mouseMovedWhileLeftButtonPressed = true;
        const QPoint& dp = e->pos() - d->lastMousePos;
        d->offset += QPoint(dp.x(), -dp.y());
        d->lastMousePos = e->pos();
        calcViewport(width(), height());
    }
    else {
        if (d->shaderProgram->isLinked()) {
            d->mousePos = QPointF(e->pos() - QPoint(d->viewport.left(), height() - d->viewport.bottom()));
            d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
            update();
        }
    }
    e->accept();
}

void RenderWidget::mousePressEvent(QMouseEvent* e)
{
    Q_D(RenderWidget);
    switch (e->button()) {
    case Qt::LeftButton:
        d->leftMouseButtonPressed = true;
        d->lastMousePos = e->pos();
        break;
    case Qt::RightButton:
        d->marks.clear();
        updateUniforms();
        break;
    default:
        break;
    }
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* e)
{
    Q_D(RenderWidget);
    switch (e->button()) {
    case Qt::LeftButton:
        if (!d->mouseMovedWhileLeftButtonPressed) {
            const QPointF& mp = e->pos() - QPoint(d->viewport.left(), height() - d->viewport.bottom());
            d->marks << QVector2D(mp.x() / d->viewport.width(), mp.y() / d->viewport.height());
            updateUniforms();
        }
        d->leftMouseButtonPressed = false;
        d->mouseMovedWhileLeftButtonPressed = false;
        break;
    case Qt::RightButton:
        d->marks.clear();
        updateUniforms();
        break;
    default:
        break;
    }
}

void RenderWidget::wheelEvent(QWheelEvent* e)
{
    Q_D(RenderWidget);
    if (d->img.isNull() || e->delta() == 0)
        return;
    double f = e->delta() * (e->modifiers() & Qt::ControlModifier)? 0.1 : 0.05;
    d->scale *= (e->delta() < 0)? 1-f : 1+f;
    calcViewport(width(), height());
}

void RenderWidget::dragEnterEvent(QDragEnterEvent* e)
{
    const QMimeData* const d = e->mimeData();
    if (d->hasUrls() && d->urls().first().toString().contains(QRegExp("\\.(png|jpg|gif|ico|mng|tga|tiff?)$", Qt::CaseInsensitive)))
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
        if (fileUrl.contains(QRegExp("file://.*\\.(png|jpg|gif|ico|mng|tga|tiff?)$", Qt::CaseInsensitive)))
#if defined(WIN32)
            loadImage(fileUrl.remove("file:///"));
#else
            loadImage(fileUrl.remove("file://"));
#endif
    }
}

bool RenderWidget::loadImage(const QString& filename)
{
    Q_D(RenderWidget);
    if (filename.isEmpty())
        return false;
    QImage image(filename);
    if (image.isNull())
        return false;
    setImage(image);
    fitImageToWindow();
    d->imgFilename = filename;
    emit imageDropped(d->img);
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

