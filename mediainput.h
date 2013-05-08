// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __VIDEOCAPTUREDEVICE_H_
#define __VIDEOCAPTUREDEVICE_H_

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QStringList>

class MediaInputPrivate;

class MediaInput : public QObject
{
    Q_OBJECT
public:
    explicit MediaInput(int id = -1, QObject* parent = NULL);
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    explicit MediaInput(const QString& filename, QObject* parent = NULL);
#endif
    ~MediaInput();
    
    bool open(int);
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    bool open(const QString& filename);
#endif

    bool isOpen(void) const;
    void close(void);
    const QImage& getLastFrame(void) const;
    const QImage& getCurrentFrame(void);
    void getRawFrame(const uchar*& data, int& w, int& h);
    QSize frameSize(void) const;
    bool setFrameSize(const QSize&);

    static bool startup(void);
    static QStringList availableDevices(void);

signals:
    
public slots:

private:
    QScopedPointer<MediaInputPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MediaInput)
    Q_DISABLE_COPY(MediaInput)

    static bool startedUp;
};

#endif // __VIDEOCAPTUREDEVICE_H_
