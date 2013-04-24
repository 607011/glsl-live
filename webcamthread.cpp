/* Copyright (c) 2011 Oliver Lau <oliver@von-und-fuer-lau.de>
 * All rights reserved.
 * $Id: webcamthread.cpp ce61935ee133 2011/06/28 16:08:54 Oliver Lau <oliver@von-und-fuer-lau.de> $
 */

#include <QtCore/QDebug>

#include "webcamthread.h"
#include "webcam.h"
#include "util.h"

class WebcamThreadPrivate {
public:
    WebcamThreadPrivate(Webcam* webcam)
        : webcam(webcam)
        , abort(false)
    { /* ... */ }
    ~WebcamThreadPrivate()
    {
        safeDelete(webcam);
    }
    Webcam* webcam;
    bool abort;
};

WebcamThread::WebcamThread(Webcam* webcam, QObject* parent)
    : QThread(parent)
    , d_ptr(new WebcamThreadPrivate(webcam))
{
    /* ... */
}

WebcamThread::~WebcamThread()
{
    stopReading();
}

void WebcamThread::startReading(void)
{
    Q_D(WebcamThread);
    Q_ASSERT(d->webcam != NULL);
    stopReading();
    d->abort = false;
    start();
}

void WebcamThread::stopReading(void)
{
    Q_D(WebcamThread);
    if (isRunning()) {
        d->abort = true;
        wait();
    }
}

void WebcamThread::run(void)
{
    Q_D(WebcamThread);
    Q_ASSERT(d->webcam != NULL);
    QImage frame;
    int framenumber = 0;
    while (!d->abort) {
        d->webcam->getFrame(frame, &framenumber);
        emit frameReady(frame);
    }
}
