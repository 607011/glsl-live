// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __RENDERER_H_
#define __RENDERER_H_

#include <QGLContext>
#include <QImage>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QScopedPointer>
#include "util.h"


class RendererPrivate;

class Renderer : public QGLContext
{
public:
    Renderer(void);
    ~Renderer();

    typedef QMap<QString, QVariant> UniformMap;

    void buildProgram(const QString& vs, const QString& fs);
    void setUniforms(const UniformMap& uniforms);
    void updateUniforms(void);
    QImage process(const QImage&);

    virtual bool create(const QGLContext* shareContext = NULL);

private:
    QScopedPointer<RendererPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Renderer)
    Q_DISABLE_COPY(Renderer)
};



#endif // __RENDERER_H_
