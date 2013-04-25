// Copyright (c) 2012 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtScript/QScriptEngine>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "glclass.h"


class GLClassPrivate {
public:
    GLClassPrivate(RenderWidget* rw, QScriptEngine* engine)
        : renderWidget(rw)
        , engine(engine)
    { /* ... */ }
    RenderWidget* renderWidget;
    QScriptEngine* engine;
};

GLClass::GLClass(RenderWidget* renderWidget, QScriptEngine* engine)
    : d_ptr(new GLClassPrivate(renderWidget, engine))
{
    /* ... */
}

GLClass::~GLClass()
{
    /* ... */
}

void GLClass::uniform1f(const QString& location, qreal v0)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), (GLfloat)v0);
}

void GLClass::uniform2f(const QString& location, qreal v0, qreal v1)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), QVector2D(v0, v1));
}

void GLClass::uniform3f(const QString& location, qreal v0, qreal v1, qreal v2)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), QVector3D(v0, v1, v2));
}

void GLClass::uniform4f(const QString& location, qreal v0, qreal v1, qreal v2, qreal v3)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), QVector4D(v0, v1, v2, v3));
}

void GLClass::uniform1i(const QString& location, int v0)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLint)v0);
}

void GLClass::uniform2i(const QString& location, int v0, int v1)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLint)v0, (GLint)v1);
}

void GLClass::uniform3i(const QString& location, int v0, int v1, int v2)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLint)v0, (GLint)v1, (GLint)v2);
}

void GLClass::uniform4i(const QString& location, int v0, int v1, int v2, int v3)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLint)v0, (GLint)v1, (GLint)v2, (GLint)v3);
}

void GLClass::uniform1ui(const QString& location, unsigned int v0)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLuint)v0);
}

void GLClass::uniform2ui(const QString& location, unsigned int v0, unsigned int v1)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLuint)v0, (GLuint)v1);
}

void GLClass::uniform3ui(const QString& location, unsigned int v0, unsigned int v1, unsigned int v2)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLuint)v0, (GLuint)v1, (GLuint)v2);
}

void GLClass::uniform4ui(const QString& location, unsigned int v0, unsigned int v1, unsigned int v2, unsigned int v3)
{
    d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(),
                                                          (GLuint)v0, (GLuint)v1, (GLuint)v2, (GLuint)v3);
}

void GLClass::uniformMatrix3fv(const QString& location, bool transpose, const QScriptValue& v)
{
    static const int Dim = 3;
    if (!v.isArray()) {
        d_ptr->engine->currentContext()->throwError(tr("Value must be an 3x3 array"));
        return;
    }
    QVector<GLfloat> data;
    const QVariantList& vl = v.toVariant().toList();
    if (vl.size() != Dim) {
        d_ptr->engine->currentContext()->throwError(tr("Value must be an 3x3 array"));
        return;
    }
    QListIterator<QVariant> row(vl);
    while (row.hasNext()) {
        const QVariant& r = row.next();
        const QVariantList& innerVL = r.toList();
        if (innerVL.size() != Dim) {
            d_ptr->engine->currentContext()->throwError(tr("Value must be an 3x3 array"));
            return;
        }
        QListIterator<QVariant> col(innerVL);
        while (col.hasNext()) {
            bool ok = false;
            float f = col.next().toFloat(&ok);
            if (!ok) {
                d_ptr->engine->currentContext()->throwError(tr("Array element must be a float value"));
                return;
            }
            data.append(f);
        }
    }
    QMatrix3x3 mat(data.data());
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), mat.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), mat);
}

#if 0
void GLClass::uniformMatrix2fv(const QString& location, bool transpose, const QMatrix2x2& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix4fv(const QString& location, bool transpose, const QMatrix4x4& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix2x3fv(const QString& location, bool transpose, const QMatrix2x3& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix3x2fv(const QString& location, bool transpose, const QMatrix3x2& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix2x4fv(const QString& location, bool transpose, const QMatrix2x4& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix4x2fv(const QString& location, bool transpose, const QMatrix4x2& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix3x4fv(const QString& location, bool transpose, const QMatrix3x4& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}

void GLClass::uniformMatrix4x3fv(const QString& location, bool transpose, const QMatrix4x3& value)
{
    if (transpose)
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value.transposed());
    else
        d_ptr->renderWidget->shaderProgram()->setUniformValue(location.toLatin1().constData(), value);
}
#endif

void GLClass::setVertexShaderSource(const QString& vs)
{
    d_ptr->renderWidget->setVertexShaderSource(vs);
}

const QString& GLClass::vertexShaderSource(void) const
{
    return d_ptr->renderWidget->vertexShaderSource();
}

void GLClass::setFragmentShaderSource(const QString& fs)
{
    d_ptr->renderWidget->setFragmentShaderSource(fs);
}

const QString& GLClass::fragmentShaderSource(void) const
{
    return d_ptr->renderWidget->fragmentShaderSource();
}

bool GLClass::build(void)
{
    return d_ptr->renderWidget->build();
}

bool GLClass::start(void)
{
    d_ptr->renderWidget->goLive();
    return true;
}

bool GLClass::stop(void)
{
    d_ptr->renderWidget->stopCode();
    return true;
}

QImage GLClass::frame(void) const
{
    return d_ptr->renderWidget->resultImage();
}
