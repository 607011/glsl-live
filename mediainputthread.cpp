// Copyright (c) 2011-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>

#include "mediainput.h"
#include "mediainputthread.h"
#include "util.h"

class MediaInputThreadPrivate {
public:
    MediaInputThreadPrivate(MediaInput* mediaIn)
        : mediaInput(mediaIn)
        , abort(false)
    { /* ... */ }
    ~MediaInputThreadPrivate()
    { /* ... */ }
    MediaInput* mediaInput;
    bool abort;
};

MediaInputThread::MediaInputThread(MediaInput* media, QObject* parent)
    : QThread(parent)
    , d_ptr(new MediaInputThreadPrivate(media))
{ /* ... */ }

MediaInputThread::~MediaInputThread()
{
    stopReading();
}

void MediaInputThread::startReading(void)
{
    Q_D(MediaInputThread);
    Q_ASSERT_X(d->mediaInput != NULL, "WebcamThread::startReading()", "d->webcam must not be NULL");
    stopReading();
    d->abort = false;
    start();
}

void MediaInputThread::stopReading(void)
{
    Q_D(MediaInputThread);
    if (isRunning()) {
        d->abort = true;
        wait();
    }
}

void MediaInputThread::run(void)
{
    Q_D(MediaInputThread);
    Q_ASSERT_X(d->mediaInput != NULL, "WebcamThread::run()", "d->webcam must not be NULL");
    while (!d->abort) {
        const uchar* data = NULL;
        if (d->mediaInput->type() == MediaInput::Video) {
            int w = -1, h = -1;
            int tries = 10;
            while (--tries && data == NULL)
                d->mediaInput->getRawFrame(data, w, h);
            emit rawFrameReady(data, w, h, Project::SourceWebcam);
        }
        else if (d->mediaInput->type() == MediaInput::Audio) {
            int length = -1;
            int tries = 10;
            while (--tries && data == NULL)
                d->mediaInput->getRawFrame(data, length);
            emit rawFrameReady(data, length, Project::SourceData);
        }
    }
}
