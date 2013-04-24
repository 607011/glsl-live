// Copyright (c) 2011-2013 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __WEBCAM_H_
#define __WEBCAM_H_

#include <QObject>
#include <QTimer>
#include <QImage>
#include <QSize>
#include <QScopedPointer>

#include "abstractvideodecoder.h"


namespace cv {
    class VideoCapture;
}

class WebcamPrivate;

class Webcam : public IAbstractVideoDecoder
{
    Q_OBJECT
public:
    Webcam(QObject* parent = NULL);
    virtual ~Webcam();

    // IAbstractVideoDecoder methods
    bool open(const char*) { return false; }

    bool open(int deviceId);
    bool isOpen(void) const;
    void close(void);
    bool seekNextFrame(int);
    bool getFrame(QImage& img, int* effectiveframenumber = 0, int* effectiveframetime = 0, int* desiredframenumber = 0, int* desiredframetime = 0);
    bool seekFrame(qint64);
    bool seekMs(int);
    QSize frameSize(void) const;
    int getVideoLengthMs(void);
    QString codecInfo(void) const;
    const QString typeName(void) const;

    // other methods
    void setSize(const QSize&);

private: // variables
    QScopedPointer<WebcamPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Webcam)
    Q_DISABLE_COPY(Webcam)
};

#endif // __WEBCAM_H_
