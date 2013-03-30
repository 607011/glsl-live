// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __PROJECT_H_
#define __PROJECT_H_

#include <QObject>
#include <QImage>
#include <QString>
#include <QRgb>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QQueue>

class Project : public QObject
{
    Q_OBJECT

public:
    explicit Project(QObject* parent = NULL);
    void reset(void);
    bool save(void);
    bool save(const QString& fileName);
    bool load(const QString& fileName);
    QString errorString(void) const;
    bool isDirty(void) const;
    const QString& filename(void) const;
    const QString vertexShaderSource(void) const;
    const QString fragmentShaderSource(void) const;
    const QImage& image(void) const;
    void setDirty(bool dirty = true);
    void setVertexShaderSource(const QString&);
    void setFragmentShaderSource(const QString&);
    void setImage(const QImage&);
    void setFilename(const QString&);

signals:

private: // variables
    bool mDirty;
    QXmlStreamReader mXml;
    QString mVertexShaderSource;
    QString mFragmentShaderSource;
    QImage mImage;
    QString mFilename;

private: // methods
    void read(void);
    bool read(QIODevice*);

    void readShaders(void);
    void readInput(void);
    void readShaderVertex(void);
    void readShaderFragment(void);
    void readInputImage(void);
    void readInputImageData(void);
    void readInputWebcam(void);
};

#endif // __PROJECT_H_
