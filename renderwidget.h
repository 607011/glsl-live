// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef RENDERWIDGET_H
#define RENDERWIDGET_H

#include <QGLWidget>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QPoint>
#include <QRgb>
#include <QPointF>
#include <QImage>
#include <QTime>
#include <QTimerEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QCursor>

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
    const QString& imageFileName(void) const { return mImageFileName; }
    bool loadImage(const QString& fileName);
    const QImage& image(void) const { return mImage; }

signals:
    void shaderError(QString);
    void linkingSuccessful(void);
    void imageDropped(const QImage&);


private:
    static const QVector3D mVertices[4];
    static const QVector2D mTexCoords[4];
    bool mInitializedGL;
    QGLShader* mVertexShader;
    QGLShader* mFragmentShader;
    QGLShaderProgram* mShaderProgram;
    GLuint mTextureHandle;
    GLenum mGLerror;
    QTime mTime;
    QString mImageFileName;
    QImage mImage;
    int mTimerId;
    QPointF mMousePos;
    QSizeF mResolution;
    QString mVertexShaderSource;
    QString mFragmentShaderSource;

private: // methods
    void goLive(void);
    void stopCode(void);
    bool linkProgram(const QString& vs = QString(), const QString& fs = QString());

protected:
    void resizeEvent(QResizeEvent*);
    void initializeGL(void);
    void resizeGL(int, int);
    void paintGL(void);
    void timerEvent(QTimerEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void dragEnterEvent(QDragEnterEvent*);
    void dragLeaveEvent(QDragLeaveEvent*);
    void dropEvent(QDropEvent*);

};

#endif // RENDERWIDGET_H
