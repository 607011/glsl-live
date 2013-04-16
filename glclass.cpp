// Copyright (c) 2012 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtScript/QScriptEngine>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "glclass.h"


/*******************************************
 *
 * GLClass
 *
 *******************************************/

GLClass::GLClass(RenderWidget* renderWidget)
    : mRenderWidget(renderWidget)
{
    /* ... */
}

GLClass::~GLClass()
{
    /* ... */
}

void GLClass::uniform1f(const QString& location, GLfloat v0)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0);
}

void GLClass::uniform2f(const QString& location, GLfloat v0, GLfloat v1)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), QVector2D(v0, v1));
}

void GLClass::uniform3f(const QString& location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), QVector3D(v0, v1, v2));
}

void GLClass::uniform4f(const QString& location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), QVector4D(v0, v1, v2, v3));
}

void GLClass::uniform1i(const QString& location, GLint v0)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0);
}

void GLClass::uniform2i(const QString& location, GLint v0, GLint v1)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0, v1);
}

void GLClass::uniform3i(const QString& location, GLint v0, GLint v1, GLint v2)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0, v1, v2);
}

void GLClass::uniform4i(const QString& location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0, v1, v2, v3);
}

void GLClass::uniform1ui(const QString& location, GLuint v0)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0);
}

void GLClass::uniform2ui(const QString& location, GLuint v0, GLuint v1)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0, v1);
}

void GLClass::uniform3ui(const QString& location, GLuint v0, GLuint v1, GLuint v2)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0, v1, v2);
}

void GLClass::uniform4ui(const QString& location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    mRenderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), v0, v1, v2, v3);
}

void GLClass::setVertexShader(const QString& vs)
{
    mRenderWidget->setVertexShader(vs);
}

const QString& GLClass::vertexShader(void) const
{
    return mRenderWidget->vertexShader();
}

void GLClass::setFragmentShader(const QString& fs)
{
    mRenderWidget->setFragmentShader(fs);
}

const QString& GLClass::fragmentShader(void) const
{
    return mRenderWidget->fragmentShader();
}
