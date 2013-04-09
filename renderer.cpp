// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QGLFramebufferObject>
#include <QGLShader>
#include <QGLShaderProgram>
#include <QPainter>
#include <QTime>

#include "util.h"
#include "renderer.h"


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


class RendererPrivate {
public:
    RendererPrivate(void)
        : textureHandle(0)
        , shaderProgram(NULL)
        , vertexShader(NULL)
        , fragmentShader(NULL)
        , fbo(NULL)
    { /* ... */ }
    ~RendererPrivate() {
        shaderProgram->removeAllShaders();
        safeDelete(shaderProgram);
        safeDelete(vertexShader);
        safeDelete(fragmentShader);
    }
    GLuint textureHandle;
    QGLShader* vertexShader;
    QGLShader* fragmentShader;
    QGLShaderProgram* shaderProgram;
    QGLFramebufferObject* fbo;
    Renderer::UniformMap uniforms;
    QTime t;
};

// Man möchte meinen, dass es genügt, von QGLContext abzuleiten,
// um OpenGL rendern zu können, aber Pustekuchen: Nur das Erben
// von QGLWidget erlaubt das Zeichnen mit einem eigenen Kontext.
Renderer::Renderer(QWidget* parent)
    : QGLWidget(parent)
    , d_ptr(new RendererPrivate)
{
    Q_D(Renderer);
    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    glGenTextures(1, &d->textureHandle);
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    d->t.start();
    doneCurrent();
}

Renderer::~Renderer()
{
    if (isValid())
        doneCurrent();
    deleteTexture(d_ptr->textureHandle);
}

void Renderer::updateUniforms(void)
{
    Q_D(Renderer);
    makeCurrent();
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
            break;
        }
    }
}

void Renderer::process(const QImage& image, const QString& outFilename)
{
    Q_D(Renderer);
    if (!isValid())
        return;
    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT);
    if (d->fbo == NULL || d->fbo->size() != image.size())
        safeRenew(d->fbo, new QGLFramebufferObject(image.size()));
    d->fbo->bind();
    glBindTexture(GL_TEXTURE_2D, d->textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
    d->shaderProgram->setUniformValue("uTexture", 0);
    d->shaderProgram->setUniformValue("uResolution", QSizeF(image.size()));
    d->shaderProgram->setUniformValue("uT", GLfloat(1e-3 * d->t.elapsed()));
    updateUniforms();
    glViewport(0, 0, image.width(), image.height());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    d->fbo->release();
    d->fbo->toImage().save(outFilename);
    doneCurrent();
}

void Renderer::buildProgram(const QString& vs, const QString& fs)
{
    Q_D(Renderer);
    if (vs.isEmpty() || fs.isEmpty())
        return;
    if (!isValid())
        return;
    makeCurrent();
    if (d->shaderProgram)
        d->shaderProgram->removeAllShaders();
    safeRenew(d->shaderProgram, new QGLShaderProgram);
    safeRenew(d->vertexShader, new QGLShader(QGLShader::Vertex));
    safeRenew(d->fragmentShader, new QGLShader(QGLShader::Fragment));
    d->shaderProgram->addShader(d->vertexShader);
    d->shaderProgram->addShader(d->fragmentShader);
    d->vertexShader->compileSourceCode(vs);
    d->fragmentShader->compileSourceCode(fs);
    d->shaderProgram->link();
    d->shaderProgram->bindAttributeLocation("aVertex", PROGRAM_VERTEX_ATTRIBUTE);
    d->shaderProgram->bindAttributeLocation("aTexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    d->shaderProgram->bind();
    d->shaderProgram->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    d->shaderProgram->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    d->shaderProgram->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, Vertices);
    d->shaderProgram->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, TexCoords);
    doneCurrent();
}

void Renderer::setUniforms(const Renderer::UniformMap& uniforms)
{
     d_ptr->uniforms = uniforms;
}
