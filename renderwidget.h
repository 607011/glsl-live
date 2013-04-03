// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERWIDGET_H_
#define __RENDERWIDGET_H_

#include <QGLWidget>
#include <QString>
#include <QImage>
#include <QTimerEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QScopedPointer>

class RenderWidgetPrivate;

class RenderWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit RenderWidget(QWidget* parent = NULL);
    ~RenderWidget();
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) const { return QSize(640, 480); }

    void setShaderSources(const QString& vs, const QString& fs);
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
    void timerEvent(QTimerEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dragLeaveEvent(QDragLeaveEvent*);
    void dropEvent(QDropEvent*);

private: // methods
    void goLive(void);
    void stopCode(void);
    void buildProgram(const QString& vs, const QString& fs);
    void makeImageFBO(void);
    void calcViewport(const QSize&);
    void calcViewport(int w, int h);

private:
    QScopedPointer<RenderWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(RenderWidget)
    Q_DISABLE_COPY(RenderWidget)

};

#endif // __RENDERWIDGET_H_
