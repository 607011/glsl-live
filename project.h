// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __PROJECT_H_
#define __PROJECT_H_

#include <QObject>
#include <QColor>
#include <QImage>
#include <QString>
#include <QIODevice>
#include <QScopedPointer>

class ProjectPrivate;

class Project : public QObject
{
    Q_OBJECT

public:
    explicit Project(QObject* parent = NULL);
    ~Project();
    void reset(void);
    bool save(void);
    bool save(const QString& filename);
    bool load(const QString& filename);
    QString errorString(void) const;
    bool isDirty(void) const;
    const QString& filename(void) const;
    const QString& vertexShaderSource(void) const;
    const QString& fragmentShaderSource(void) const;
    const QString& scriptSource(void) const;
    const QImage& image(void) const;
    void setDirty(bool dirty = true);
    void setClean(bool clean = true);
    void setVertexShaderSource(const QString&);
    void setFragmentShaderSource(const QString&);
    void setScriptSource(const QString&);
    void setImage(const QImage&);
    void setFilename(const QString&);
    bool alphaEnabled(void) const;
    bool imageRecyclingEnabled(void) const;
    bool instantUpdateEnabled(void) const;
    bool borderClampingEnabled(void) const;
    const QColor& backgroundColor(void) const;

public slots:
    void enableAlpha(bool enabled = true);
    void enableImageRecycling(bool enabled = true);
    void enableInstantUpdate(bool enabled = true);
    void enableBorderClamping(bool enabled = true);
    void setBackgroundColor(const QColor&);

private: // methods
    void read(void);
    bool read(QIODevice*);

    void readShaders(void);
    void readScript(void);
    void readInput(void);
    void readShaderVertex(void);
    void readShaderFragment(void);
    void readInputImage(void);
    void readOptions(void);
    void readAlpha(void);
    void readClamp(void);
    void readBackgroundColor(void);
    void readInstantUpdate(void);

    void resetErrors(void);
    void raiseError(const QString&);

private: // variables
    QScopedPointer<ProjectPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Project)
    Q_DISABLE_COPY(Project)
};

#endif // __PROJECT_H_
