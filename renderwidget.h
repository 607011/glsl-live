// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERWIDGET_H_
#define __RENDERWIDGET_H_

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
#include <QMap>
#include <QVariant>

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
    void setUniformValue(const QString& name, int value);
    void setUniformValue(const QString& name, double value);
    void setUniformValue(const QString& name, bool value);
    void updateUniforms();
    void clearUniforms(void) { mUniforms.clear(); }

signals:
    void shaderError(QString);
    void linkingSuccessful(void);
    void imageDropped(const QImage&);

private:
    static const QVector3D mVertices[4];
    static const QVector2D mTexCoords[4];
    bool mFirstPaintEventPending;
    QGLShader* mVertexShader;
    QGLShader* mFragmentShader;
    QGLShaderProgram* mShaderProgram;
    GLuint mTextureHandle;
    GLenum mGLerror;
    QTime mTime;
    QString mImageFileName;
    QImage mImage;
    int mLiveTimerId;
    QPointF mMousePos;
    QSizeF mResolution;
    QString mPreliminaryVertexShaderSource;
    QString mPreliminaryFragmentShaderSource;
    QString mVertexShaderSource;
    QString mFragmentShaderSource;
    int mULocT;
    int mULocMouse;
    int mULocResolution;
    int mULocTexture;
    QMap<QString, QVariant> mUniforms;

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

#endif // __RENDERWIDGET_H_
