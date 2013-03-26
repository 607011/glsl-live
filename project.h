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
    bool save(const QString& fileName);
    bool load(const QString& fileName);
    QString errorString(void) const;

    void setVertexShaderSource(const QString& source) { mVertexShaderSource = source; }
    void setFragmentShaderSource(const QString& source) { mFragmentShaderSource = source; }
    void setImage(const QImage& image) { mImage = image; }

signals:

private: // variables
    QXmlStreamReader mXml;
    QString mVertexShaderSource;
    QString mFragmentShaderSource;
    QImage mImage;
    int mWebcam;
    QQueue<ParameterWidget> mParameterWidgets;
    ParameterWidget mCurrentParameterWidget;

private: // methods
    const QString vertexShaderSource(void) const { return mVertexShaderSource; }
    const QString fragmentShaderSource(void) const { return mFragmentShaderSource; }
    const QImage& image(void) const { return mImage; }
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
    void readParameterSlider(void);
    void readParameterSliderDirection(void);
    void readShaderVertex(void);
    void readShaderFragment(void);
    void readInputImage(void);
    void readInputImageData(void);
    void readInputWebcam(void);
};

#endif // __PROJECT_H_
