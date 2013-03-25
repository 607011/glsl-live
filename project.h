// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QRgb>
#include <QIODevice>
#include <QXmlStreamReader>

class Project : public QObject
{
    Q_OBJECT

public:
    explicit Project(QObject* parent = NULL);
    bool save(const QString& fileName);
    bool load(const QString& fileName);
    QString errorString(void) const;

signals:
    void vertexShaderFound(QString);
    void fragmentShaderFound(QString);
    void imageFound(QImage);

public slots:

private:
    QXmlStreamReader mXml;
    void read(void);
    bool read(QIODevice*);

    void readShaders(void);
    void readInput(void);
    void readParameters(void);
    void readVertexShader(void);
    void readFragmentShader(void);
    void readImage(void);
    void readImageData(void);
    void readWebcam(void);
};

#endif // PROJECT_H
