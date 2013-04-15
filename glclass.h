#ifndef __GLCLASS_H_
#define __GLCLASS_H_

#include <QObject>
#include <QString>
#include <QtScript/QScriptClass>
#include <QtScript/QScriptString>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptable>
#include <QtGui/qopengl.h>

#include "renderwidget.h"

class GLClass : public QObject, public QScriptClass
{
    Q_OBJECT
public:
    explicit GLClass(RenderWidget* renderWidget = NULL, QScriptEngine* engine = NULL);
    ~GLClass();

    QScriptValue constructor(void);
    QScriptValue newInstance(void);
    QString name(void) const;
    QScriptValue prototype(void) const;

    static void Init(RenderWidget* renderWidget, QScriptEngine* engine)
    {
        engine->globalObject().setProperty("gl", (new GLClass(renderWidget, engine))->constructor());
    }

private:
    static QScriptValue construct(QScriptContext* ctx, QScriptEngine* eng);

    QScriptValue mProto;
    QScriptValue mCtor;
};


class GLPrototype : public QObject, public QScriptable
{
    Q_OBJECT

public:
    GLPrototype(RenderWidget* renderWidget, QObject* parent = NULL);
    ~GLPrototype();

public slots:
    void uniform1f(const char* location, GLfloat v0);
    void uniform2f(const char* location, GLfloat v0, GLfloat v1);
    void uniform3f(const char* location, GLfloat v0, GLfloat v1, GLfloat v2);
    void uniform4f(const char* location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void uniform1i(const char* location, GLint v0);
    void uniform2i(const char* location, GLint v0, GLint v1);
    void uniform3i(const char* location, GLint v0, GLint v1, GLint v2);
    void uniform4i(const char* location, GLint v0, GLint v1, GLint v2, GLint v3);
    void uniform1ui(const char* location, GLuint v0);
    void uniform2ui(const char* location, GLuint v0, GLuint v1);
    void uniform3ui(const char* location, GLuint v0, GLuint v1, GLuint v2);
    void uniform4ui(const char* location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

private:
    RenderWidget* mRenderWidget;
};

Q_DECLARE_METATYPE(GLClass*)

#endif // __GLCLASS_H_
