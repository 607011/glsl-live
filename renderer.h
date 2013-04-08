#ifndef __RENDERER_H_
#define __RENDERER_H_

#include <QGLContext>
#include <QGLFramebufferObject>
#include <QGLShader>
#include <QGLShaderProgram>
#include <QImage>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QTime>
#include "util.h"

class Renderer : public QGLContext
{
public:
    Renderer(void);
    Renderer(const Renderer&);
    ~Renderer();
    void init(void);

    typedef QMap<QString, QVariant> UniformMap;

    void buildProgram(const QString& vs, const QString& fs);
    void setUniforms(const UniformMap& uniforms) { mUniforms = uniforms; }
    void updateUniforms(void);
    QImage process(const QImage&);
    void makeCurrent(void);

private:
    GLuint mTextureHandle;
    QGLShader* mVertexShader;
    QGLShader* mFragmentShader;
    QGLShaderProgram* mShaderProgram;
    QGLFramebufferObject* mFBO;
    UniformMap mUniforms;
    QTime mT;
};



#endif // __RENDERER_H_
