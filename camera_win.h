// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __CAMERA_WIN_H_
#define __CAMERA_WIN_H_

#include <QObject>
#include <QImage>
#include <QScopedPointer>

class WebcamPrivate;

class Webcam : public QObject
{
    Q_OBJECT
public:
    explicit Webcam(QObject* parent = NULL);
    ~Webcam();
    bool open(int);
    bool isOpen(void) const;
    void close(void);
    const QImage& getFrame(void);
    void getRawFrame(const uchar*& data, int& w, int& h) const;
    QSize frameSize(void) const;
    void setSize(const QSize&);

signals:
    
public slots:

private:
    QScopedPointer<WebcamPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Webcam)
    Q_DISABLE_COPY(Webcam)

};

#endif // __CAMERA_WIN_H_
