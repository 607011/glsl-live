// Copyright (c) 2012 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include "scriptrunner.h"
#include "glclass.h"
#include "imageclass.h"
#include "util.h"

class ScriptRunnerPrivate {
public:
    ScriptRunnerPrivate(RenderWidget* renderWidget)
        : renderWidget(renderWidget)
        , scriptEngine(new QScriptEngine)
    { /* ... */ }
    ~ScriptRunnerPrivate()
    {
        safeDelete(scriptEngine);
    }
    QScriptEngine* scriptEngine;
    RenderWidget* renderWidget;
    QString scriptSource;
};

ScriptRunner::ScriptRunner(RenderWidget* renderWidget)
    : d_ptr(new ScriptRunnerPrivate(renderWidget))
{
    Q_D(ScriptRunner);
    // set up scripting engine
    GLClass::Init(d->renderWidget, d->scriptEngine);
    ImageClass::Init(d->scriptEngine);
}

ScriptRunner::~ScriptRunner()
{
    /* ... */
}

const QScriptEngine* ScriptRunner::engine(void) const
{
    return d_ptr->scriptEngine;
}

QScriptEngine* ScriptRunner::engine(void)
{
    return d_ptr->scriptEngine;
}

void ScriptRunner::execute(const QString& source)
{
    d_ptr->scriptSource = source;
    run();
}

void ScriptRunner::run(void)
{
    Q_D(ScriptRunner);
    QScriptValue& globals = d->scriptEngine->globalObject();
    globals.setProperty("W", d->renderWidget->width());
    globals.setProperty("H", d->renderWidget->height());
    const QScriptValue& result = d->scriptEngine->evaluate(d->scriptSource);
    if (result.isError()) {
        emit debug(tr("%1 (line: %2)").arg(result.toString()).arg(result.property("lineNumber").toInt32()));
    }
    else if (d->scriptEngine->hasUncaughtException()) {
        emit debug(d->scriptEngine->uncaughtException().toString());
        QStringListIterator ex(d->scriptEngine->uncaughtExceptionBacktrace());
        while (ex.hasNext())
            emit debug(QString("  %1").arg(ex.next()));
    }
}
