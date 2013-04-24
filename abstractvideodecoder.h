// Copyright (c) 2011-2013 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __IABSTRACTVIDEODECODER_H_
#define __IABSTRACTVIDEODECODER_H_

#include <QImage>

class IAbstractVideoDecoder : public QObject
{
public:
    IAbstractVideoDecoder(QObject* parent = NULL)
        : QObject(parent)
        , mDefaultSkip(1)
    { /* ... */ }
    virtual ~IAbstractVideoDecoder()
    { /* ... */ }

public: // virtual methods
    virtual bool open(const char* fileName) = 0;
    virtual bool open(int deviceId) = 0;
    virtual bool isOpen(void) const = 0;
    virtual void close(void) = 0;
    virtual bool seekMs(int) = 0;
    virtual bool seekFrame(qint64 frame) = 0;
    virtual bool seekNextFrame(int skip) = 0;
    virtual bool getFrame(QImage& img, int* effectiveframenumber, int* effectiveframetime = 0, int* desiredframenumber = 0, int* desiredframetime = 0) = 0;
    virtual QSize frameSize(void) const = 0;
    virtual int getVideoLengthMs(void) = 0;
    virtual QString codecInfo(void) const = 0;
    virtual const QString typeName(void) const = 0;

public: // method impl
    int getDefaultSkip(void) const { return mDefaultSkip; }

protected:
    int mDefaultSkip;

};

#endif // __IABSTRACTVIDEODECODER_H_
