// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __SCRIPTRUNNER_H_
#define __SCRIPTRUNNER_H_

#include <QtScript/QScriptEngine>
#include <QScopedPointer>
#include <QVector>
#include <QString>
#include <QThread>

#include "renderwidget.h"
#include "util.h"

class ScriptRunnerPrivate;

class ScriptRunner : public QThread
{
    Q_OBJECT
public:
    explicit ScriptRunner(RenderWidget*);
    ~ScriptRunner();

    const QScriptEngine* engine(void) const;
    QScriptEngine* engine(void);

public slots:
    void execute(const QString& source);

signals:
    void debug(const QString& message);

protected: // methods
    virtual void run(void);

private:
    QScopedPointer<ScriptRunnerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ScriptRunner)
    Q_DISABLE_COPY(ScriptRunner)

};


#endif // __SCRIPTRUNNER_H_
