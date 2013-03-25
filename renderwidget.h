// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef RENDERWIDGET_H
#define RENDERWIDGET_H

#include <QGLWidget>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QVector2D>
#include <QVector3D>
#include <QPoint>
#include <QRgb>
#include <QPointF>
#include <QImage>
#include <QTime>
#include <QTimerEvent>
#include <QMouseEvent>

class RenderWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit RenderWidget(QWidget* parent = NULL);
    ~RenderWidget();
    QSize minimumSizeHint(void) const { return QSize(320, 240); }
    QSize sizeHint(void) const { return QSize(640, 480); }

    void setShaderSources(const QString& vs, const QString& fs);

private:
    static const QVector3D mVertices[4];
    static const QVector2D mTexCoords[4];
    QGLShaderProgram* mShaderProgram;
    GLuint mTextureHandle;
    GLenum mGLerror;
    QTime mTime;
    QImage mImage;
    int mTimerId;
    QPointF mMousePos;

private: // methods
    void goLive(void);
    void stopCode(void);

    void setupShaderProgram(void);

protected:
    void initializeGL(void);
    void resizeGL(int, int);
    void paintGL(void);
    void timerEvent(QTimerEvent*);
    void mouseMoveEvent(QMouseEvent*);
    
};

#endif // RENDERWIDGET_H
