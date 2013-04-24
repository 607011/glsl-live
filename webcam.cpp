// Copyright (c) 2011-2013 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QtCore/QDebug>
#include <opencv/cv.h>
#include <opencv/cv.hpp>
#include <opencv/highgui.h>

#include "webcam.h"
#include "util.h"

class WebcamPrivate
{
public:
    WebcamPrivate(void)
        : webcam(NULL)
        , frameNumber(0)
        , frameTime(0)
    { /* ... */ }
    ~WebcamPrivate()
    {
        safeDelete(webcam);
    }
    cv::VideoCapture* webcam;
    QImage lastFrame;
    QSize frameSize;
    int frameNumber;
    int frameTime;
    cv::Mat frame;
};

Webcam::Webcam(QObject* parent)
    : IAbstractVideoDecoder(parent)
    , d_ptr(new WebcamPrivate)
{
    /* ... */
}

Webcam::~Webcam()
{
    close();
}

bool Webcam::open(int deviceId)
{
    Q_D(Webcam);
    close();
    d->webcam = new cv::VideoCapture(deviceId);
    if (d->webcam->isOpened()) {
        cv::Mat frame;
        d->webcam->read(frame);
        d->frameSize = QSize((int)d->webcam->get(CV_CAP_PROP_FRAME_WIDTH), (int)d->webcam->get(CV_CAP_PROP_FRAME_HEIGHT));
        return !d->frameSize.isNull();
    }
    return false;
}

bool Webcam::isOpen(void) const
{
    return d_ptr->webcam->isOpened();
}

void Webcam::close(void)
{
    Q_D(Webcam);
    if (d->webcam && d->webcam->isOpened())
        d->webcam->release();
}

bool Webcam::seekNextFrame(int)
{
    Q_D(Webcam);
    Q_ASSERT(d->webcam != NULL);
    d->webcam->read(d->frame);
    const int w = d->frame.cols;
    const int h = d->frame.rows;
    if (d->lastFrame.size() != QSize(w, h))
        d->lastFrame = QImage(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y)
        memcpy(d->lastFrame.scanLine(y), d->frame.ptr(y), 3*w);
    ++d->frameNumber;
    ++d->frameTime; // XXX
    return true;
}

bool Webcam::getFrame(QImage& img, int* effectiveframenumber, int* effectiveframetime, int*, int*)
{
    Q_D(Webcam);
    seekNextFrame(0);
    img = d->lastFrame;
    if (effectiveframenumber)
        *effectiveframenumber = d->frameNumber;
    if (effectiveframetime)
        *effectiveframetime = d->frameTime;
    return true;
}

void Webcam::getRawFrame(const uchar*& data, int& w, int& h) const
{
    if (d_ptr->webcam && d_ptr->webcam->isOpened()) {
        d_ptr->webcam->read(d_ptr->frame);
        w = d_ptr->frame.cols;
        h = d_ptr->frame.rows;
        data = d_ptr->frame.data;
    }
    else {
        data = NULL;
        w = 0;
        h = 0;
    }
}

void Webcam::setSize(const QSize& sz)
{
    Q_D(Webcam);
    Q_ASSERT(sz.isValid());
    Q_ASSERT(d->webcam != NULL);
    d->webcam->set(CV_CAP_PROP_FRAME_WIDTH, sz.width());
    d->webcam->set(CV_CAP_PROP_FRAME_HEIGHT, sz.height());
}

bool Webcam::seekFrame(qint64) { return false; }
bool Webcam::seekMs(int) { return false; }
QSize Webcam::frameSize() const { return d_ptr->frameSize; }
int Webcam::getVideoLengthMs(void) { return -1; }
QString Webcam::codecInfo(void) const { return QString(); }
const QString Webcam::typeName(void) const { return "Webcam"; }
