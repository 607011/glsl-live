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
#include <qmath.h>
#include "renderwidget.h"
#include "channelwidget.h"
#include "project.h"
#include "util.h"

enum { AVERTEX, ATEXCOORD };

static const QVector2D TexCoords[4] =
{
    QVector2D(1, 0),
    QVector2D(1, 1),
    QVector2D(0, 0),
    QVector2D(0, 1)
};
static const QVector2D TexCoords4FBO[4] =
{
    QVector2D(1, 1),
    QVector2D(1, 0),
    QVector2D(0, 1),
    QVector2D(0, 0)
};
static const QVector2D Vertices[4] =
{
    QVector2D( 1.0,  1.0),
    QVector2D( 1.0, -1.0),
    QVector2D(-1.0,  1.0),
    QVector2D(-1.0, -1.0)
};


class RenderWidgetPrivate {
public:
    explicit RenderWidgetPrivate(void)
        : alphaEnabled(true)
        , clampToBorder(true)
        , imageRecyclingEnabled(false)
        , goAheadOneFrame(false)
        , instantUpdate(false)
        , firstPaintEventPending(true)
        , vertexShader(NULL)
        , fragmentShader(NULL)
        , shaderProgram(NULL)
        , fbo(NULL)
        , liveTimerId(0)
        , scale(1.0)
        , leftMouseButtonPressed(false)
        , mouseMovedWhileLeftButtonPressed(false)
        , mouseMoveTimerId(0)
        , framesElapsedCount(0)
        , glVersionMajor(0)
        , glVersionMinor(0)
    { /* ... */ }
    QColor backgroundColor;
    bool alphaEnabled;
    bool clampToBorder;
    bool imageRecyclingEnabled;
    bool goAheadOneFrame;
    bool instantUpdate;
    bool firstPaintEventPending;
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QGLFramebufferObject* fbo;
    GLuint textureHandle;
    GLuint channelHandle[Project::MAX_CHANNELS];
    QTime totalTime;
    QTime frameTime;
    QString imgFilename;
    QImage img;
    QImage channel[Project::MAX_CHANNELS];
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
    int uLocChannel[Project::MAX_CHANNELS];
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
    qint64 framesElapsedCount;
    GLint glVersionMajor;
    GLint glVersionMinor;

    static const double Friction;
    static const int TimeInterval;
    static const int NumKineticDataSamples;

    bool shaderProgramIsLinked(void) const
    {
        return (shaderProgram != NULL) && shaderProgram->isLinked();
    }

    void makeShaderProgram(void)
    {
        deleteShaderProgram();
        deleteShaders();
        try {
            shaderProgram = new QGLShaderProgram;
            vertexShader = new QGLShader(QGLShader::Vertex);
            fragmentShader = new QGLShader(QGLShader::Fragment);
        }
        catch (...) {
            qFatal("memory allocation error");
        }
        shaderProgram->addShader(vertexShader);
        shaderProgram->addShader(fragmentShader);
    }
    virtual ~RenderWidgetPrivate()
    {
        deleteShaderProgram();
        deleteShaders();
        safeDelete(fbo);
    }

private:
    void deleteShaders(void)
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
    : QGLWidget(QGLFormat(QGL::SingleBuffer | QGL::NoDepthBuffer | QGL::AlphaChannel | QGL::NoAccumBuffer | QGL::NoStencilBuffer | QGL::NoStereoBuffers | QGL::HasOverlay | QGL::NoSampleBuffers), parent)
    , d_ptr(new RenderWidgetPrivate)
{
    Q_D(RenderWidget);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setAcceptDrops(true);
    setCursor(Qt::OpenHandCursor);
    d->totalTime.start();
    d->frameTime.start();
}

RenderWidget::~RenderWidget()
{
    stopMotion();
    stopCode();
}

void RenderWidget::enableAlpha(bool enabled)
{
    Q_D(RenderWidget);
    makeCurrent();
    d->alphaEnabled = enabled;
    if (d->alphaEnabled) {
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
    Q_D(RenderWidget);
    d->imageRecyclingEnabled = enabled;
    update();
}

void RenderWidget::enableInstantUpdate(bool enabled)
{
    Q_D(RenderWidget);
    d->instantUpdate = enabled;
    update();
}

void RenderWidget::setBackgroundColor(const QColor& color)
{
    Q_D(RenderWidget);
    makeCurrent();
    d->backgroundColor = color;
    qglClearColor(color);
}

void RenderWidget::feedbackOneFrame(void)
{
    Q_D(RenderWidget);
    d->goAheadOneFrame = true;
    update();
}

void RenderWidget::setTimerActive(bool active)
{
    if (active)
        goLive();
    else
        stopCode();
}

void RenderWidget::clampToBorder(bool enabled)
{
    Q_D(RenderWidget);
    d->clampToBorder = enabled;
    configureTexture();
}

void RenderWidget::setShaderSources(const QString& vs, const QString& fs)
{
    Q_D(RenderWidget);
    d->preliminaryVertexShaderSource = vs;
    d->preliminaryFragmentShaderSource = fs;
    buildProgram(d->preliminaryVertexShaderSource, d->preliminaryFragmentShaderSource);
    if (d->shaderProgramIsLinked()) {
        d->vertexShaderSource = d->preliminaryVertexShaderSource;
        d->fragmentShaderSource = d->preliminaryFragmentShaderSource;
        emit linkingSuccessful();
    }
    else {
        buildProgram(d->vertexShaderSource, d->fragmentShaderSource);
    }
    if (d->shaderProgramIsLinked()) {
        updateUniforms();
        goLive();
    }
}

void RenderWidget::setVertexShaderSource(const QString& vs)
{
    Q_D(RenderWidget);
    d->vertexShaderSource = vs;
}

const QString& RenderWidget::vertexShaderSource(void) const
{
    return d_ptr->vertexShaderSource;
}

void RenderWidget::setFragmentShaderSource(const QString& fs)
{
    Q_D(RenderWidget);
    d->fragmentShaderSource = fs;
}

const QString& RenderWidget::fragmentShaderSource(void) const
{
    return d_ptr->fragmentShaderSource;
}

void RenderWidget::makeImageFBO(void)
{
    Q_D(RenderWidget);
    makeCurrent();
    if (d->fbo == NULL || d->fbo->size() != d->img.size())
        safeRenew(d->fbo, new QGLFramebufferObject(d->img.size()));
}

void RenderWidget::setImage(const QImage& img)
{
    Q_D(RenderWidget);
    if (!img.isNull() ) {
        d->img = img.convertToFormat(QImage::Format_ARGB32);
        makeImageFBO();
    }
    makeCurrent();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->img.width(), d->img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->img.bits());
}

void RenderWidget::setChannel(int index, const uchar* data, int w, int h)
{
    Q_ASSERT_X(index >= 0 && index < Project::MAX_CHANNELS, "RenderWidget::setChannel()", "image index out of bounds");
    Q_D(RenderWidget);
    makeCurrent();
    glActiveTexture(GL_TEXTURE1 + index);
    glBindTexture(GL_TEXTURE_2D, d->channelHandle[index]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    update();
}

void RenderWidget::setChannel(int index, const QImage& img)
{
    Q_ASSERT_X(index >= 0 && index < Project::MAX_CHANNELS, "RenderWidget::setChannel()", "image index out of bounds");
    Q_D(RenderWidget);
    d->channel[index] = img.convertToFormat(QImage::Format_ARGB32);
    makeCurrent();
    glActiveTexture(GL_TEXTURE1 + index);
    glBindTexture(GL_TEXTURE_2D, d->channelHandle[index]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->channel[index].width(), d->channel[index].height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, d->channel[index].bits());
    update();
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
    if (!d->shaderProgramIsLinked())
        return;
    d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
    d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    d->shaderProgram->setUniformValue(d->uLocTexture, 0);
    for (int i = 0; i < Project::MAX_CHANNELS; ++i)
        d->shaderProgram->setUniformValue(d->uLocChannel[i], 1 + i);
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
        case QVariant::Color:
            d->shaderProgram->setUniformValue(key.toUtf8().data(), value.value<QColor>());
            break;
        default:
            qWarning() << "RenderWidget::updateUniforms(): invalid value type in d->uniforms";
            break;
        }
    }
    update();
}

void RenderWidget::clearUniforms(void)
{
    d_ptr->uniforms.clear();
}

bool RenderWidget::build(void)
{
    Q_D(RenderWidget);
    if (d->vertexShaderSource.isEmpty() || d->fragmentShaderSource.isEmpty())
        return false;
    buildProgram(d->vertexShaderSource, d->fragmentShaderSource);
    return d->shaderProgramIsLinked();
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
    d->shaderProgram->bindAttributeLocation("aVertex", AVERTEX);
    d->shaderProgram->bindAttributeLocation("aTexCoord", ATEXCOORD);
    d->shaderProgram->bind();
    d->shaderProgram->enableAttributeArray(AVERTEX);
    d->shaderProgram->enableAttributeArray(ATEXCOORD);
    d->shaderProgram->setAttributeArray(AVERTEX, Vertices);

    d->uLocT = d->shaderProgram->uniformLocation("uT");
    d->uLocTexture = d->shaderProgram->uniformLocation("uTexture");
    for (int i = 0; i < Project::MAX_CHANNELS; ++i)
        d->uLocChannel[i] = d->shaderProgram->uniformLocation(ChannelWidget::channelName(i));
    d->uLocMouse = d->shaderProgram->uniformLocation("uMouse");
    d->uLocResolution = d->shaderProgram->uniformLocation("uResolution");
    d->uLocMarks = d->shaderProgram->uniformLocation("uMarks");
    d->uLocMarksCount = d->shaderProgram->uniformLocation("uMarksCount");
}

void RenderWidget::goLive(void)
{
    Q_D(RenderWidget);
    if (d->liveTimerId == 0) {
        d->liveTimerId = startTimer(1000/50);
    }
}

void RenderWidget::stopCode(void)
{
    Q_D(RenderWidget);
    if (d->liveTimerId != 0) {
        killTimer(d_ptr->liveTimerId);
        d->liveTimerId = 0;
    }
}

void RenderWidget::updateViewport(void)
{
    updateViewport(width(), height());
}

QString RenderWidget::glVersionString() const
{
    return QString("%1.%2").arg(d_ptr->glVersionMajor).arg(d_ptr->glVersionMinor);
}

bool RenderWidget::isTimerActive(void) const
{
    return d_ptr->liveTimerId != 0;
}

QGLShaderProgram* RenderWidget::shaderProgram(void)
{
    return d_ptr->shaderProgram;
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
    if (d->shaderProgramIsLinked())
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


void RenderWidget::configureTexture(void)
{
    Q_D(RenderWidget);
    makeCurrent();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    if (d->clampToBorder) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
}

void RenderWidget::resizeGL(int w, int h)
{
    updateViewport(w, h);
}

void RenderWidget::initializeGL(void)
{
    Q_D(RenderWidget);
    initializeGLFunctions();
    glGetIntegerv(GL_MAJOR_VERSION, &d->glVersionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &d->glVersionMinor);
    qglClearColor(d->backgroundColor);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glGenTextures(1, &d->textureHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    configureTexture();
    glGenTextures(Project::MAX_CHANNELS, d->channelHandle);
    for (int i = 0; i < Project::MAX_CHANNELS; ++i) {
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, d->channelHandle[i]);
        configureTexture();
    }
}

void RenderWidget::paintGL(void)
{
    Q_D(RenderWidget);
    ++d->framesElapsedCount;
    if (d->frameTime.elapsed() > 500) {
        emit fpsChanged(d->framesElapsedCount * 1e3 / d->frameTime.elapsed());
        d->frameTime.start();
        d->framesElapsedCount = 0;
    }
    if (d->firstPaintEventPending) {
        glGetIntegerv(GL_MAJOR_VERSION, &d->glVersionMajor);
        glGetIntegerv(GL_MINOR_VERSION, &d->glVersionMinor);
        enableAlpha(d->alphaEnabled);
        buildProgram(d->preliminaryVertexShaderSource, d->preliminaryFragmentShaderSource);
        if (d->shaderProgramIsLinked())
            goLive();
        setImage();
        d->firstPaintEventPending = false;
        emit ready();
    }

    if (d->liveTimerId == 0)
        return;

    if (d->shaderProgramIsLinked()) {
        d->shaderProgram->bind();
        d->shaderProgram->setUniformValue(d->uLocT, 1e-3f * (GLfloat)d->totalTime.elapsed());
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // draw onto screen
    d->shaderProgram->setUniformValue(d->uLocResolution, d->resolution);
    d->shaderProgram->setAttributeArray(ATEXCOORD, TexCoords);
    glViewport(d->viewport.x(), d->viewport.y(), d->viewport.width(), d->viewport.height());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (d->imageRecyclingEnabled || d->goAheadOneFrame) {
        // draw into FBO
        glViewport(0, 0, d->fbo->width(), d->fbo->height());
        d->shaderProgram->setUniformValue(d->uLocResolution, QSizeF(d->fbo->size()));
        d->shaderProgram->setAttributeArray(ATEXCOORD, TexCoords4FBO);
        d->fbo->bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, d->fbo->width(), d->fbo->height(), 0);
        d->fbo->release();
        d->goAheadOneFrame = false;
    }

    emit frameReady();
    if (d->instantUpdate)
        update();
}

void RenderWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->modifiers() & Qt::AltModifier) {
        setCursor(Qt::ArrowCursor);
    }
    e->accept();
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
    e->accept();
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
        if (d->shaderProgramIsLinked()) {
            d->mousePos = QPointF(e->pos() - QPoint(d->viewport.left(), height() - d->viewport.bottom()));
            d->shaderProgram->setUniformValue(d->uLocMouse, d->mousePos);
            emit mousePosChanged(d->mousePos);
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
    e->accept();
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
    e->accept();
}

void RenderWidget::wheelEvent(QWheelEvent* e)
{
    Q_D(RenderWidget);
    if (d->img.isNull() || e->delta() == 0)
        return;
    double f = e->delta() * (e->modifiers() & Qt::ControlModifier)? 0.1 : 0.05;
    d->scale *= (e->delta() < 0)? 1-f : 1+f;
    updateViewport();
    e->accept();
}

void RenderWidget::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls() && e->mimeData()->urls().first().toString().contains(QRegExp("\\.(png|jpg|jpeg|gif|ico|mng|tga|tiff?)$", Qt::CaseInsensitive)))
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
    if (e->mimeData()->hasUrls()) {
        QString fileUrl = e->mimeData()->urls().first().toString();
        if (fileUrl.contains(QRegExp("file://.*\\.(png|jpg|jpeg|gif|ico|mng|tga|tiff?)$", Qt::CaseInsensitive)))
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

void RenderWidget::setUniformValue(const QString& name, const QColor& value)
{
    d_ptr->uniforms[name] = value;
    updateUniforms();
}

void RenderWidget::setUniforms(const QMap<QString, QVariant>& uniforms)
{
    Q_D(RenderWidget);
    QStringListIterator k(uniforms.keys());
    while (k.hasNext()) {
        const QString& key = k.next();
        d->uniforms[key] = uniforms[key];
    }
    if (!uniforms.isEmpty() && !d->uniforms.isEmpty())
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
