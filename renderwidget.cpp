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
        : alphaEnabled(true)
        , imageRecyclingEnabled(false)
        , instantUpdate(false)
        , firstPaintEventPending(true)
        , vertexShader(NULL)
        , fragmentShader(NULL)
        , shaderProgram(new QGLShaderProgram)
        , fbo(NULL)
        , imgData(NULL)
        , inputTextureHandle(0)
        , liveTimerId(0)
        , scale(1.0)
        , leftMouseButtonPressed(false)
        , mouseMovedWhileLeftButtonPressed(false)
        , mouseMoveTimerId(0)
        , nFrame(0)
    { /* ... */ }
    QColor backgroundColor;
    bool alphaEnabled;
    bool imageRecyclingEnabled;
    bool instantUpdate;
    bool firstPaintEventPending;
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QGLFramebufferObject* fbo;
    GLuint* imgData;
    GLuint inputTextureHandle;
    QTime totalTime;
    QTime frameTime;
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

    struct KineticData {
        KineticData(void) : t(0) { /* ... */ }
        KineticData(const QPoint& p, int t) : p(p), t(t) { /* ... */ }
        QPoint p;
        int t;
    };

    QVector<KineticData> kineticData;
    QTime mouseMoveTimer;
    int mouseMoveTimerId;
    QPointF velocity;
    qint64 nFrame;

    static const double Friction;
    static const int TimeInterval;
    static const int NumKineticDataSamples;

    void makeShaderProgram(void)
    {
        deleteShaderProgram();
        deleteShaders();
        shaderProgram = new QGLShaderProgram();
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
        safeDelete(imgData);
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

const double RenderWidgetPrivate::Friction = 0.81;
const int RenderWidgetPrivate::TimeInterval = 25;
const int RenderWidgetPrivate::NumKineticDataSamples = 4;


RenderWidget::RenderWidget(QWidget* parent)
    : QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::NoDepthBuffer | QGL::AlphaChannel | QGL::NoAccumBuffer | QGL::NoStencilBuffer | QGL::NoStereoBuffers | QGL::HasOverlay | QGL::NoSampleBuffers), parent)
    , d_ptr(new RenderWidgetPrivate)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setAcceptDrops(true);
    setCursor(Qt::OpenHandCursor);
    d_ptr->totalTime.start();
    d_ptr->frameTime.start();
}

RenderWidget::~RenderWidget()
{
    stopMotion();
    stopCode();
}

void RenderWidget::enableAlpha(bool enabled)
{
    makeCurrent();
    d_ptr->alphaEnabled = enabled;
    if (d_ptr->alphaEnabled) {
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glDisable(GL_BLEND);
    }
    update();
}

void RenderWidget::enableImageRecycling(bool enabled)
{
    d_ptr->imageRecyclingEnabled = enabled;
    update();
}

void RenderWidget::enableInstantUpdate(bool enabled)
{
    d_ptr->instantUpdate = enabled;
    update();
}

void RenderWidget::setBackgroundColor(const QColor& color)
{
    makeCurrent();
    d_ptr->backgroundColor = color;
    qglClearColor(color);
}

void RenderWidget::setShaderSources(const QString& vs, const QString& fs)
{
    Q_D(RenderWidget);
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
        safeRenew(d->fbo, new QGLFramebufferObject(d->img.size()));
        safeRenew(d->imgData, new GLuint[d->fbo->width() * d->fbo->height()]);
    }
}

void RenderWidget::setImage(const QImage& img)
{
    Q_D(RenderWidget);
    if (!img.isNull()) {
        d->img = img.convertToFormat(QImage::Format_ARGB32);
        makeImageFBO();
    }
    makeCurrent();
    glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
}

const QString& RenderWidget::imageFileName(void) const
{
    return d_ptr->imgFilename;
}

QImage RenderWidget::resultImage(void)
{
    Q_D(RenderWidget);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    makeImageFBO();
    d->fbo->bind();
    d->shaderProgram->setUniformValue(d->uLocResolution, QSizeF(d->img.size()));
    glViewport(0, 0, d->img.width(), d->img.height());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    d->fbo->release();
    glPopAttrib();
    d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    return d->fbo->toImage();
}

const QMap<QString, QVariant>& RenderWidget::uniforms(void) const
{
    return d_ptr->uniforms;
}

double RenderWidget::scale(void) const
{
    return d_ptr->scale;
}

void RenderWidget::updateUniforms(void)
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
    if (d_ptr->liveTimerId == 0) {
        d_ptr->liveTimerId = startTimer(1000/50);
    }
}

void RenderWidget::stopCode()
{
    if (d_ptr->liveTimerId != 0) {
        killTimer(d_ptr->liveTimerId);
        d_ptr->liveTimerId = 0;
    }
}

void RenderWidget::updateViewport(void)
{
    updateViewport(width(), height());
}

void RenderWidget::updateViewport(const QSize& size)
{
    updateViewport(size.width(), size.height());
}

void RenderWidget::updateViewport(int w, int h)
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
    updateViewport(e->size());
}

void RenderWidget::setScale(double scale)
{
    Q_D(RenderWidget);
    d->scale = scale;
    updateViewport();
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
    updateViewport();
}

void RenderWidget::resizeToOriginalImageSize(void)
{
    Q_D(RenderWidget);
    d->offset = QPoint();
    setScale(1.0);
}

void RenderWidget::resizeGL(int w, int h)
{
    updateViewport(w, h);
    updateGL();
}

void RenderWidget::initializeGL(void)
{
    Q_D(RenderWidget);
    qglClearColor(d->backgroundColor);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glGenTextures(1, &d->inputTextureHandle);
    glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void RenderWidget::paintGL(void)
{
    Q_D(RenderWidget);
    static const qint64 FRAMESTEP = 10;
    if (d->nFrame++ % FRAMESTEP == 0) {
        emit fpsChanged(FRAMESTEP * 1e3 / d->frameTime.elapsed());
        d->frameTime.start();
    }
    glClear(GL_COLOR_BUFFER_BIT);
    if (d->firstPaintEventPending) {
        enableAlpha(d->alphaEnabled);
        buildProgram(d->preliminaryVertexShaderSource, d->preliminaryFragmentShaderSource);
        if (d->shaderProgram->isLinked())
            goLive();
        setImage();
        d->firstPaintEventPending = false;
    }
    if (d->shaderProgram->isLinked())
        d->shaderProgram->setUniformValue(d->uLocT, 1e-3f * (GLfloat)d->totalTime.elapsed());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    if (d->imageRecyclingEnabled) {
        // draw into FBO
        d->fbo->bind();
        glViewport(0, 0, d->fbo->width(), d->fbo->height());
        d->shaderProgram->setUniformValue(d->uLocResolution, QSizeF(d->fbo->size()));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // retrieve pixel data from FBO
        glReadPixels(0, 0, d->fbo->width(), d->fbo->height(), GL_BGRA, GL_UNSIGNED_BYTE, d->imgData);
        d->fbo->release();

        // load FBO into texture
        glBindTexture(GL_TEXTURE_2D, d->inputTextureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->fbo->width(), d->fbo->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->imgData);

        // restore viewport and uResolution
        glViewport(d->viewport.x(), d->viewport.y(), d->viewport.width(), d->viewport.height());
        d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    }
    if (d->instantUpdate)
        update();
}

void RenderWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->modifiers() & Qt::AltModifier) {
        setCursor(Qt::ArrowCursor);
    }
}

void RenderWidget::keyReleaseEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        stopMotion();
        resizeToOriginalImageSize();
        break;
    default:
        break;
    }
    setCursor(Qt::OpenHandCursor);
}

void RenderWidget::timerEvent(QTimerEvent* e)
{
    Q_D(RenderWidget);
    if (e->timerId() == d->liveTimerId) {
        updateGL();
    }
    else if (e->timerId() == d->mouseMoveTimerId) {
        if (d->velocity.manhattanLength() > M_SQRT2) {
            scrollBy(d->velocity.toPoint());
            d->velocity *= RenderWidgetPrivate::Friction;
        }
        else stopMotion();
    }
    else
        qWarning() << "RenderWidget::timerEvent() received bad timer id:" << e->timerId();
    e->accept();
}

void RenderWidget::mouseMoveEvent(QMouseEvent* e)
{
    Q_D(RenderWidget);
    setFocus(Qt::MouseFocusReason);
    if (d->leftMouseButtonPressed) {
        d->mouseMovedWhileLeftButtonPressed = true;
        const QPoint& dp = e->pos() - d->lastMousePos;
        d->offset += QPoint(dp.x(), -dp.y());
        d->kineticData.append(RenderWidgetPrivate::KineticData(e->pos(), d->mouseMoveTimer.elapsed()));
        if (d->kineticData.size() > RenderWidgetPrivate::NumKineticDataSamples)
            d->kineticData.erase(d->kineticData.begin());
        updateViewport();
        d->lastMousePos = e->pos();
        setCursor((e->modifiers() & Qt::AltModifier)? Qt::ArrowCursor : Qt::ClosedHandCursor);
    }
    else {
        if (d->shaderProgram->isLinked()) {
            d->mousePos = QPointF(e->pos() - QPoint(d->viewport.left(), height() - d->viewport.bottom()));
            d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
            update();
        }
        setCursor((e->modifiers() & Qt::AltModifier)? Qt::ArrowCursor : Qt::OpenHandCursor);
    }
    e->accept();
}

void RenderWidget::mousePressEvent(QMouseEvent* e)
{
    Q_D(RenderWidget);
    switch (e->button()) {
    case Qt::LeftButton:
        d->leftMouseButtonPressed = true;
        if ((e->modifiers() & Qt::AltModifier) == 0) {
            stopMotion();
            d->mouseMoveTimer.start();
            d->kineticData.clear();
            d->lastMousePos = e->pos();
        }
        setCursor((e->modifiers() & Qt::AltModifier)? Qt::ArrowCursor : Qt::ClosedHandCursor);
        break;
    case Qt::RightButton:
        if (e->modifiers() & Qt::AltModifier) {
            d->marks.clear();
            updateUniforms();
        }
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
            if (e->modifiers() & Qt::AltModifier) {
                const QPointF& mp = e->pos() - QPoint(d->viewport.left(), height() - d->viewport.bottom());
                d->marks.append(QVector2D(mp.x() / d->viewport.width(), mp.y() / d->viewport.height()));
                updateUniforms();
            }
        }
        else {
            if (d->kineticData.count() == RenderWidgetPrivate::NumKineticDataSamples) {
                int timeSinceLastMoveEvent = d->mouseMoveTimer.elapsed() - d->kineticData.last().t;
                if (timeSinceLastMoveEvent < 100) {
                    int dt = d->mouseMoveTimer.elapsed() - d->kineticData.first().t;
                    const QPointF& moveDist = d->kineticData.first().p - e->pos();
                    const QPointF& initialVector = 1000 * moveDist / dt / RenderWidgetPrivate::TimeInterval;
                    startMotion(initialVector);
                }
            }
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
    setCursor((e->modifiers() & Qt::AltModifier)? Qt::ArrowCursor : Qt::OpenHandCursor);
}

void RenderWidget::wheelEvent(QWheelEvent* e)
{
    Q_D(RenderWidget);
    if (d->img.isNull() || e->delta() == 0)
        return;
    double f = e->delta() * (e->modifiers() & Qt::ControlModifier)? 0.1 : 0.05;
    d->scale *= (e->delta() < 0)? 1-f : 1+f;
    updateViewport();
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

void RenderWidget::startMotion(const QPointF& velocity)
{
    Q_D(RenderWidget);
    d->velocity = velocity;
    if (d->mouseMoveTimerId == 0)
        d->mouseMoveTimerId = startTimer(RenderWidgetPrivate::TimeInterval);
}

void RenderWidget::stopMotion(void)
{
    Q_D(RenderWidget);
    if (d->mouseMoveTimerId) {
        killTimer(d->mouseMoveTimerId);
        d->mouseMoveTimerId = 0;
    }
    d->velocity = QPointF();
}

void RenderWidget::scrollBy(const QPoint& offset)
{
    Q_D(RenderWidget);
    d->offset -= QPoint(offset.x(), -offset.y());
    updateViewport();
}
