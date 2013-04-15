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

GLClass::GLClass(RenderWidget* renderWidget, QScriptEngine* engine)
    : QObject(engine)
    , QScriptClass(engine)
{
    mProto = engine->newQObject(new GLPrototype(renderWidget, this),
                                QScriptEngine::QtOwnership,
                                QScriptEngine::SkipMethodsInEnumeration
                                | QScriptEngine::ExcludeSuperClassMethods
                                | QScriptEngine::ExcludeSuperClassProperties);
    const QScriptValue& global = engine->globalObject();
    mProto.setPrototype(global.property("Object").property("prototype"));
    mCtor = engine->newFunction(construct, mProto);
    mCtor.setData(engine->toScriptValue(this));
}


GLClass::~GLClass()
{
    /* ... */
}


QString GLClass::name(void) const
{
    return QLatin1String("GL");
}


QScriptValue GLClass::prototype(void) const
{
    return mProto;
}


QScriptValue GLClass::constructor(void)
{
    return mCtor;
}


QScriptValue GLClass::construct(QScriptContext* ctx, QScriptEngine*)
{
    GLClass* cls = qscriptvalue_cast<GLClass*>(ctx->callee().data());
    if (!cls)
        return QScriptValue();
    return cls->newInstance();
}


QScriptValue GLClass::newInstance(void)
{
    return engine()->newObject(this);
}




/*******************************************
 *
 * GLPrototype
 *
 *******************************************/


GLPrototype::GLPrototype(RenderWidget* renderWidget, QObject* parent)
    : QObject(parent)
    , mRenderWidget(renderWidget)
{
    /* ... */
}


GLPrototype::~GLPrototype()
{
    /* ... */
}

void GLPrototype::uniform1f(const char* location, GLfloat v0)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0);
}

void GLPrototype::uniform2f(const char* location, GLfloat v0, GLfloat v1)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, QVector2D(v0, v1));
}

void GLPrototype::uniform3f(const char* location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, QVector3D(v0, v1, v2));
}

void GLPrototype::uniform4f(const char* location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, QVector4D(v0, v1, v2, v3));
}

void GLPrototype::uniform1i(const char* location, GLint v0)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0);
}

void GLPrototype::uniform2i(const char* location, GLint v0, GLint v1)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0, v1);
}

void GLPrototype::uniform3i(const char* location, GLint v0, GLint v1, GLint v2)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0, v1, v2);
}

void GLPrototype::uniform4i(const char* location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0, v1, v2, v3);
}

void GLPrototype::uniform1ui(const char* location, GLuint v0)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0);
}

void GLPrototype::uniform2ui(const char* location, GLuint v0, GLuint v1)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0, v1);
}

void GLPrototype::uniform3ui(const char* location, GLuint v0, GLuint v1, GLuint v2)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0, v1, v2);
}

void GLPrototype::uniform4ui(const char* location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    mRenderWidget->shaderProgram()->setUniformValue(location, v0, v1, v2, v3);
}
