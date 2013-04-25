// Copyright (c) 2011-2013 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __WEBCAM_H_
#define __WEBCAM_H_

#include <QObject>
#include <QTimer>
#include <QImage>
#include <QSize>
#include <QScopedPointer>

namespace cv {
    class VideoCapture;
}

class WebcamPrivate;

class Webcam : public QObject
{
    Q_OBJECT
public:
    Webcam(QObject* parent = NULL);
    ~Webcam();

    // IAbstractVideoDecoder methods
    bool open(int deviceId);
    bool isOpen(void) const;
    void close(void);
    const QImage& getFrame(void);
    void getRawFrame(const uchar*& data, int& w, int& h) const;
    QSize frameSize(void) const;

    // other methods
    void setSize(const QSize&);

private: // variables
    QScopedPointer<WebcamPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Webcam)
    Q_DISABLE_COPY(Webcam)
};

#endif // __WEBCAM_H_
