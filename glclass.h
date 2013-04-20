#ifndef __GLCLASS_H_
#define __GLCLASS_H_

#ifdef ENABLED_SCRIPTING

#include <QObject>
#include <QString>
#include <QtScript/QScriptClass>
#include <QtScript/QScriptString>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptable>
#include <QtGui/qopengl.h>
#include <QMatrix2x2>
#include <QMatrix2x3>
#include <QMatrix2x4>
#include <QMatrix3x2>
#include <QMatrix3x3>
#include <QMatrix3x4>
#include <QMatrix4x2>
#include <QMatrix4x3>
#include <QMatrix4x4>
#include <QScopedPointer>

#include "renderwidget.h"

class GLClassPrivate;

class GLClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString vertexshader READ vertexShaderSource WRITE setVertexShaderSource)
    Q_PROPERTY(QString fragmentshader READ fragmentShaderSource WRITE setFragmentShaderSource)
    Q_PROPERTY(QImage frame READ frame)

public:
    explicit GLClass(RenderWidget* renderWidget);
    ~GLClass();

    static void Init(RenderWidget* renderWidget, QScriptEngine* engine)
    {
        Q_ASSERT(renderWidget != NULL);
        engine->globalObject().setProperty("gl", engine->newQObject(new GLClass(renderWidget)));
    }

public slots:
    void uniform1f(const QString& location, qreal v0);
    void uniform2f(const QString& location, qreal v0, qreal v1);
    void uniform3f(const QString& location, qreal v0, qreal v1, qreal v2);
    void uniform4f(const QString& location, qreal v0, qreal v1, qreal v2, qreal v3);
    void uniform1i(const QString& location, int v0);
    void uniform2i(const QString& location, int v0, int v1);
    void uniform3i(const QString& location, int v0, int v1, int v2);
    void uniform4i(const QString& location, int v0, int v1, int v2, int v3);
    void uniform1ui(const QString& location, unsigned int v0);
    void uniform2ui(const QString& location, unsigned int v0, unsigned int v1);
    void uniform3ui(const QString& location, unsigned int v0, unsigned int v1, unsigned int v2);
    void uniform4ui(const QString& location, unsigned int v0, unsigned int v1, unsigned int v2, unsigned int v3);
    void uniformMatrix2fv(const QString& location, bool transpose, const QMatrix2x2& value);
    void uniformMatrix3fv(const QString& location, bool transpose, const QMatrix3x3& value);
    void uniformMatrix4fv(const QString& location, bool transpose, const QMatrix4x4& value);
    void uniformMatrix2x3fv(const QString& location, bool transpose, const QMatrix2x3& value);
    void uniformMatrix3x2fv(const QString& location, bool transpose, const QMatrix3x2& value);
    void uniformMatrix2x4fv(const QString& location, bool transpose, const QMatrix2x4& value);
    void uniformMatrix4x2fv(const QString& location, bool transpose, const QMatrix4x2& value);
    void uniformMatrix3x4fv(const QString& location, bool transpose, const QMatrix3x4& value);
    void uniformMatrix4x3fv(const QString& location, bool transpose, const QMatrix4x3& value);

    void setVertexShaderSource(const QString&);
    const QString& vertexShaderSource(void) const;
    void setFragmentShaderSource(const QString&);
    const QString& fragmentShaderSource(void) const;

    bool build(void);
    bool start(void);
    bool stop(void);

    QImage frame(void) const;

private:
    QScopedPointer<GLClassPrivate> d_ptr;

    Q_DECLARE_PRIVATE(GLClass)
    Q_DISABLE_COPY(GLClass)
};


Q_DECLARE_METATYPE(GLClass*)

#endif

#endif // __GLCLASS_H_

