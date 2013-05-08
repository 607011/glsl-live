// Copyright (c) 2011-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>

#include "videocapturedevice.h"
#include "webcamthread.h"
#include "util.h"

class WebcamThreadPrivate {
public:
    WebcamThreadPrivate(VideoCaptureDevice* webcam)
        : webcam(webcam)
        , abort(false)
    { /* ... */ }
    ~WebcamThreadPrivate()
    { /* ... */ }
    VideoCaptureDevice* webcam;
    bool abort;
};

WebcamThread::WebcamThread(VideoCaptureDevice* webcam, QObject* parent)
    : QThread(parent)
    , d_ptr(new WebcamThreadPrivate(webcam))
{ /* ... */ }

WebcamThread::~WebcamThread()
{
    stopReading();
}

void WebcamThread::startReading(void)
{
    Q_D(WebcamThread);
    Q_ASSERT_X(d->webcam != NULL, "WebcamThread::startReading()", "d->webcam must not be NULL");
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
    Q_ASSERT_X(d->webcam != NULL, "WebcamThread::run()", "d->webcam must not be NULL");
    while (!d->abort) {
        int w = -1, h = -1;
        const uchar* data = NULL;
        int tries = 10;
        while (--tries && data == NULL)
            d->webcam->getRawFrame(data, w, h);
        emit rawFrameReady(data, w, h, Project::SourceWebcam);
    }
}
