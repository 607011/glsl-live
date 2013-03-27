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
    explicit Project(QString filename = QString(), QObject* parent = NULL);
    void reset(void);
    bool save(void);
    bool save(const QString& fileName);
    bool load(const QString& fileName);
    QString errorString(void) const { return QObject::tr("%1 (line %2, column %3)").arg(mXml.errorString()).arg(mXml.lineNumber()).arg(mXml.columnNumber()); }
    bool isDirty(void) const { return mDirty; }
    const QString& filename(void) const { return mFilename; }
    const QString vertexShaderSource(void) const { return mVertexShaderSource; }
    const QString fragmentShaderSource(void) const { return mFragmentShaderSource; }
    const QImage& image(void) const { return mImage; }
    int webcam(void) const { return mWebcam; }

    void setDirty(bool dirty = true)
    {
        mDirty = dirty;
    }
    void setVertexShaderSource(const QString& source)
    {
        mVertexShaderSource = source;
        mDirty = true;
    }
    void setFragmentShaderSource(const QString& source)
    {
        mFragmentShaderSource = source;
        mDirty = true;
    }
    void setImage(const QImage& image)
    {
        mImage = image;
        mDirty = true;
    }
    void setFilename(const QString& filename)
    {
        mFilename = filename;
        mDirty = true;
    }

signals:

private: // variables
    bool mDirty;
    QXmlStreamReader mXml;
    QString mVertexShaderSource;
    QString mFragmentShaderSource;
    QImage mImage;
    int mWebcam;
    QString mFilename;
    QSize mImageSize;

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
