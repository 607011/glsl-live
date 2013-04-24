// Copyright (c) 2011-2013 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __WEBCAMTHREAD_H_
#define __WEBCAMTHREAD_H_

#include <QObject>
#include <QThread>
#include <QImage>
#include <QScopedPointer>

class Webcam;
class WebcamThreadPrivate;

class WebcamThread : public QThread
{
    Q_OBJECT
public:
    WebcamThread(Webcam* webcam = NULL, QObject* parent = NULL);
    ~WebcamThread();
    void startReading(void);
    void stopReading(void);

signals:
    void frameReady(QImage);

protected:
    void run(void);

private: // methods
    QScopedPointer<WebcamThreadPrivate> d_ptr;
    Q_DECLARE_PRIVATE(WebcamThread)
    Q_DISABLE_COPY(WebcamThread)
};

#endif // __WEBCAMTHREAD_H_
