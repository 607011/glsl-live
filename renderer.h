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

    typedef QMap<QString, QVariant> UniformMap;

    void buildProgram(const QString& vs, const QString& fs);
    void setUniforms(const UniformMap& uniforms);
    void process(const QImage& inputImage, const QString& outFilename);

private: // methods
    void updateUniforms(void);

private: // members
    QScopedPointer<RendererPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Renderer)
    Q_DISABLE_COPY(Renderer)
};



#endif // __RENDERER_H_
