// Copyright (c) 2011-2013 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __MEDIAINPUTTHREAD_H_
#define __MEDIAINPUTTHREAD_H_

#include <QObject>
#include <QThread>
#include <QImage>
#include <QScopedPointer>

#include "project.h"

class MediaInputThreadPrivate;
class MediaInput;

class MediaInputThread : public QThread
{
    Q_OBJECT
public:
    MediaInputThread(MediaInput* = NULL, QObject* parent = NULL);
    ~MediaInputThread();
    void startReading(void);
    void stopReading(void);

signals:
    void frameReady(QImage);
    void rawFrameReady(const uchar* data, int w, int h, Project::SourceSelector);
    void rawFrameReady(const uchar* data, int length, Project::SourceSelector);

protected:
    void run(void);

private: // methods
    QScopedPointer<MediaInputThreadPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MediaInputThread)
    Q_DISABLE_COPY(MediaInputThread)
};

#endif // __MEDIAINPUTTHREAD_H_
