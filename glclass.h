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

class GLClass : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString vertexshader READ vertexShader WRITE setVertexShader)
    Q_PROPERTY(QString fragmentshader READ fragmentShader WRITE setFragmentShader)

public:
    explicit GLClass(RenderWidget* renderWidget = NULL);
    ~GLClass();

    static void Init(RenderWidget* renderWidget, QScriptEngine* engine)
    {
        engine->globalObject().setProperty("gl", engine->newQObject(new GLClass(renderWidget)));
    }

public slots:
    void uniform1f(const QString& location, GLfloat v0);
    void uniform2f(const QString& location, GLfloat v0, GLfloat v1);
    void uniform3f(const QString& location, GLfloat v0, GLfloat v1, GLfloat v2);
    void uniform4f(const QString& location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void uniform1i(const QString& location, GLint v0);
    void uniform2i(const QString& location, GLint v0, GLint v1);
    void uniform3i(const QString& location, GLint v0, GLint v1, GLint v2);
    void uniform4i(const QString& location, GLint v0, GLint v1, GLint v2, GLint v3);
    void uniform1ui(const QString& location, GLuint v0);
    void uniform2ui(const QString& location, GLuint v0, GLuint v1);
    void uniform3ui(const QString& location, GLuint v0, GLuint v1, GLuint v2);
    void uniform4ui(const QString& location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

    void setVertexShader(const QString&);
    const QString& vertexShader(void) const;
    void setFragmentShader(const QString&);
    const QString& fragmentShader(void) const;


private:
    RenderWidget* mRenderWidget;
};


Q_DECLARE_METATYPE(GLClass*)

#endif // __GLCLASS_H_

