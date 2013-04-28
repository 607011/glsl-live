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
#include <QMap>
#include <QVariant>

class ProjectPrivate;

class Project : public QObject
{
    Q_OBJECT

public:
    static const int MAX_CHANNELS = 8;

    enum SourceSelector {
        SourceNone = 0,
        SourceData,
        SourceWebcam
    };

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
    const QVariant& channel(int index) const;
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
    bool hasImage(void) const;
    void setUniforms(const QMap<QString, QVariant>&);
    const QMap<QString, QVariant>& uniforms(void) const;

    static bool isEmpty(const QImage&);

public slots:
    void setChannel(int index, const QImage&);
    void setChannel(int index, SourceSelector, int id = 0);
    void enableAlpha(bool enabled = true);
    void enableImageRecycling(bool enabled = true);
    void enableInstantUpdate(bool enabled = true);
    void enableBorderClamping(bool enabled = true);
    void setBackgroundColor(const QColor&);

signals:
    void dirtyStateChanged(bool);

private: // methods
    void read(void);
    bool read(QIODevice*);

    void readShaders(void);
    void readScript(void);
    void readInput(void);
    void readShaderVertex(void);
    void readShaderFragment(void);
    void readInputImage(void);
    void readInputChannel(void);
    void readOptions(void);
    void readUniforms(void);
    void readUniform(void);
    void readAlpha(void);
    void readClamp(void);
    void readBackgroundColor(void);
    void readInstantUpdate(void);
    void readImageRecycling(void);

    void resetErrors(void);
    void raiseError(const QString&);

private: // variables
    QScopedPointer<ProjectPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Project)
    Q_DISABLE_COPY(Project)
};

#endif // __PROJECT_H_
