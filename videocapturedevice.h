// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __VIDEOCAPTUREDEVICE_H_
#define __VIDEOCAPTUREDEVICE_H_

#include <QObject>
#include <QScopedPointer>
#include <QStringList>

class VideoCaptureDevicePrivate;

class VideoCaptureDevice : public QObject
{
    Q_OBJECT
public:
    explicit VideoCaptureDevice(int id = -1, QObject* parent = NULL);
    ~VideoCaptureDevice();
    
    bool open(int);
    bool isOpen(void) const;
    void close(void);
    const QImage& getLastFrame(void) const;
    const QImage& getCurrentFrame(void);
    void getRawFrame(const uchar*& data, int& w, int& h);
    QSize frameSize(void) const;
    bool requestFrameSize(const QSize&);

    static bool startup(void);
    static QStringList enumerate(void);

signals:
    
public slots:

private:
    QScopedPointer<VideoCaptureDevicePrivate> d_ptr;
    Q_DECLARE_PRIVATE(VideoCaptureDevice)
    Q_DISABLE_COPY(VideoCaptureDevice)

    static bool startedUp;
};

#endif // __VIDEOCAPTUREDEVICE_H_
