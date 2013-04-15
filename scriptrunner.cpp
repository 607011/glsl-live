// Copyright (c) 2012 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include "scriptrunner.h"
#include "glclass.h"

class ScriptRunnerPrivate {
public:
    ScriptRunnerPrivate(void)
        : stopped(false)
        , prestart(true)
    { /* ... */ }
    ~ScriptRunnerPrivate()
    { /* ... */ }
    QScriptEngine scriptEngine;
    volatile bool prestart;
    volatile bool stopped;
};

ScriptRunner::ScriptRunner(RenderWidget* renderWidget)
    : d_ptr(new ScriptRunnerPrivate)
{
    // set up scripting engine
    GLClass::Init(renderWidget, &d_ptr->scriptEngine);

    // make sure that Qt's threading system is up and running to prevent lagged thread execution after pressing "run" the first time
    start();
}

ScriptRunner::~ScriptRunner()
{
    /* ... */
}

void ScriptRunner::stop(void)
{
    d_ptr->stopped = true;
}

void ScriptRunner::resume(void)
{
    d_ptr->stopped = false;
    start();
}

const QScriptEngine& ScriptRunner::engine(void) const
{
    return d_ptr->scriptEngine;
}

QScriptEngine& ScriptRunner::engine(void)
{
    return d_ptr->scriptEngine;
}

void ScriptRunner::run(void)
{
    Q_D(ScriptRunner);
    if (d->prestart) {
        d->prestart = false;
        return;
    }
}
