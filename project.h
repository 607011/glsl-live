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

#include "parameterwidget.h"

class Project : public QObject
{
    Q_OBJECT

public:
    explicit Project(QString filename = QString(), QObject* parent = NULL);
    void reset(void);
    bool save(void);
    bool save(const QString& fileName);
    bool load(const QString& fileName);
    QString errorString(void) const;
    bool isDirty(void) const { return mDirty; }
    const QString& filename(void) const { return mFilename; }
    const QString vertexShaderSource(void) const { return mVertexShaderSource; }
    const QString fragmentShaderSource(void) const { return mFragmentShaderSource; }
    const QImage& image(void) const { return mImage; }

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
    QQueue<ParameterWidget> mParameterWidgets;
    ParameterWidget mCurrentParameterWidget;
    QString mFilename;
    QSize mImageSize;

private: // methods
    int webcam(void) const { return mWebcam; }
    const QQueue<ParameterWidget>& widgets(void) const { return mParameterWidgets; }

    void read(void);
    bool read(QIODevice*);

    void readShaders(void);
    void readInput(void);
    void readParameters(void);
    void readParameter(void);
    void readParameterType(void);
    void readParameterName(void);
    void readParameterMinValue(void);
    void readParameterMaxValue(void);
    void readParameterDefaultValue(void);
    void readShaderVertex(void);
    void readShaderFragment(void);
    void readInputImage(void);
    void readInputImageData(void);
    void readInputWebcam(void);
};

#endif // __PROJECT_H_
