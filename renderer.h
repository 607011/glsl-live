// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERER_H_
#define __RENDERER_H_

#include <QGLWidget>
#include <QGLContext>
#include <QImage>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QScopedPointer>
#include <QPaintEvent>


class RendererPrivate;

class Renderer : public QGLWidget
{
public:
    Renderer(QWidget* parent = NULL);
    ~Renderer();

    virtual QSize minimumSizeHint(void) const { return QSize(240, 160); }
    virtual QSize sizeHint(void) const { return QSize(240, 160); }

    typedef QMap<QString, QVariant> UniformMap;

    void buildProgram(const QString& vs, const QString& fs);
    void setUniforms(const UniformMap& uniforms);
    void updateUniforms(void);
    const QImage& process(const QImage&);

protected:
    virtual void paintEvent(QPaintEvent*);

private:
    QScopedPointer<RendererPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Renderer)
    Q_DISABLE_COPY(Renderer)
};



#endif // __RENDERER_H_
