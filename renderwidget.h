// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERWIDGET_H_
#define __RENDERWIDGET_H_

#include <QGLWidget>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QImage>
#include <QTimerEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QScopedPointer>

class RenderWidgetPrivate;

class RenderWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit RenderWidget(QWidget* parent = NULL);
    ~RenderWidget();

    QSize minimumSizeHint(void) const { return QSize(240, 160); }
    QSize sizeHint(void) const  { return QSize(640, 480); }
    void setShaderSources(const QString& vs, const QString& fs);
    bool tryToGoLive(void);
    void setImage(const QImage&);
    const QString& imageFileName(void) const;
    bool loadImage(const QString& fileName);
    const QImage& image(void) const;
    void setUniformValue(const QString& name, int value);
    void setUniformValue(const QString& name, double value);
    void setUniformValue(const QString& name, bool value);
    void updateUniforms();
    void clearUniforms(void);
    QImage resultImage(void);

public slots:
    void fitImageToWindow(void);
    void resizeToOriginalImageSize(void);

signals:
    void vertexShaderError(QString);
    void fragmentShaderError(QString);
    void linkerError(QString);
    void linkingSuccessful(void);
    void imageDropped(const QImage&);

protected:
    void resizeEvent(QResizeEvent*);
    void initializeGL(void);
    void resizeGL(int, int);
    void paintGL(void);
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dragLeaveEvent(QDragLeaveEvent*);
    void dropEvent(QDropEvent*);
    void timerEvent(QTimerEvent*);

private: // methods
    void goLive(void);
    void stopCode(void);
    void buildProgram(const QString& vs, const QString& fs);
    void makeImageFBO(void);
    void calcViewport(void);
    void calcViewport(const QSize&);
    void calcViewport(int w, int h);
    void scrollBy(const QPoint&);
    void startMotion(const QPointF& velocity);
    void stopMotion(void);

private:
    QScopedPointer<RenderWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(RenderWidget)
    Q_DISABLE_COPY(RenderWidget)

};

#endif // __RENDERWIDGET_H_
