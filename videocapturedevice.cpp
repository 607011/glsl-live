// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QImage>
#include "videocapturedevice.h"
#include "util.h"

#if defined(WIN32)
#include <windows.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mfidl.h>
// #include <strsafe.h>
// #include <dbt.h>
#include <ks.h>
#include <ksmedia.h>
#include <dcommon.h>
#include <D2DErr.h>
#include <D2DBaseTypes.h>
#include <dxgiformat.h>

#endif // defined(WIN32)


class VideoCaptureDevicePrivate
{
public:
    VideoCaptureDevicePrivate(void)
        : frameData(NULL)
#if defined(WIN32)
        , hr(S_OK)
        , mediaType(NULL)
        , mediaPlayer(NULL)
        , mediaSource(NULL)
        , sourceReader(NULL)
#endif
    {
        /* ... */
    }

    ~VideoCaptureDevicePrivate()
    {
#if defined(WIN32)
        for (DWORD i = 0; i < camCount; i++)
            SafeRelease(&camDevices[i]);
        CoTaskMemFree(camDevices);
#endif
        safeDelete(frameData);
    }

#if defined(WIN32)
    HRESULT hr;
    IMFMediaType* mediaType;
    IMFPMediaPlayer* mediaPlayer;
    IMFMediaSource* mediaSource;
    IMFSourceReader* sourceReader;
    IMFActivate** camDevices;
    IMFAttributes* devAttr;
    UINT32 camCount;
#endif

    uchar* frameData;
    QSize frameSize;
    QImage lastFrame;

    void close(void)
    {
#if defined(WIN32)
        SafeRelease(&sourceReader);
        SafeRelease(&mediaSource);
        SafeRelease(&mediaPlayer);
#else
        // ...
#endif
    }
};


bool VideoCaptureDevice::startedUp = false;


VideoCaptureDevice::VideoCaptureDevice(int id, QObject* parent)
    : QObject(parent)
    , d_ptr(new VideoCaptureDevicePrivate)
{
    if (id >= 0)
        open(id);
}

VideoCaptureDevice::~VideoCaptureDevice()
{
    /* ... */
}

bool VideoCaptureDevice::open(int deviceId)
{
    Q_D(VideoCaptureDevice);
    close();

#if defined(WIN32)
    HRESULT hr;
    hr = MFCreateAttributes(&d->devAttr, 1);
    if (SUCCEEDED(hr))
        hr = d->devAttr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (SUCCEEDED(hr))
        hr = MFEnumDeviceSources(d->devAttr, &d->camDevices, &d->camCount);
    if (FAILED(hr)) {
        qWarning() << "Cannot create the video capture device";
        return false;
    }
    SafeRelease(&d->devAttr);
    if (deviceId <= int(d->camCount)) {
        d->camDevices[deviceId]->ActivateObject(__uuidof(IMFMediaSource),(void**)&d->mediaSource);
        hr = MFCreateSourceReaderFromMediaSource(d->mediaSource, NULL, &d->sourceReader);
    }
    if (FAILED(hr))
        return false;
    hr = d->sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &d->mediaType);
    if (FAILED(hr))
        return false;
    GUID subtype = { 0 };
    hr = d->mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (subtype != MFVideoFormat_RGB24)
        return false;
    bool ok = requestFrameSize(QSize(1920, 1080));
    qDebug() << "requestFrameSize(QSize(1920, 1080)) ->" << ok;
    return SUCCEEDED(hr) && ok;
#else
    return false;
#endif
}

bool VideoCaptureDevice::isOpen(void) const
{
#if defined(WIN32)
    return d_ptr->sourceReader != NULL;
#else
    return false;
#endif
}

void VideoCaptureDevice::close(void)
{
    Q_D(VideoCaptureDevice);
    d->close();
}

const QImage& VideoCaptureDevice::getLastFrame(void) const
{
    qDebug() << "frame size =" << d_ptr->lastFrame.size();
    return d_ptr->lastFrame;
}

const QImage& VideoCaptureDevice::getCurrentFrame(void)
{
    Q_D(VideoCaptureDevice);
    int w = -1, h = -1;
    const uchar* data = NULL;
    int Tries = 10;
    while (data == NULL && --Tries)
        getRawFrame(data, w, h);
    if (data != NULL && w > 0 && h > 0) {
        if (QSize(w, h) != d->lastFrame.size())
            d->lastFrame = QImage(w, h, QImage::Format_ARGB32);
        uchar* dst = const_cast<uchar*>(d->lastFrame.constBits());
        const uchar* src = const_cast<uchar*>(data);
        const uchar* const srcEnd = src + w * h * 3;
        while (src < srcEnd) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = 0xffU;
        }
    }
    return getLastFrame();
}

void VideoCaptureDevice::getRawFrame(const uchar*& data, int& w, int& h)
{
    Q_D(VideoCaptureDevice);
    data = NULL;
    w = -1;
    h = -1;
#if defined(WIN32)
    IMFMediaBuffer* pBuffer = NULL;
    IMFSample* pSample = NULL;
    BYTE* pBitmapData = NULL;
    DWORD cbBitmapData = 0;
    GUID subtype = { 0 };
    d->mediaType = NULL;
    DWORD dwFlags = 0x00000000U;
    HRESULT hr = d->sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &dwFlags, NULL, &pSample);
    if (FAILED(hr) || (pSample == NULL) || ((dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0))
        goto done;
    hr = d->sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &d->mediaType);
    if (FAILED(hr))
        goto done;
    hr = d->mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (subtype != MFVideoFormat_RGB24)
        goto done;
    hr = MFGetAttributeSize(d->mediaType, MF_MT_FRAME_SIZE, (UINT32*)&w, (UINT32*)&h);
    if (FAILED(hr))
        goto done;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr))
        goto done;
    hr = pBuffer->Lock(&pBitmapData, NULL, &cbBitmapData);
    if (FAILED(hr))
        goto done;
    if (cbBitmapData > 0 && w > 0 && h > 0) {
        if (d->frameSize != QSize(w, h)) {
            d->frameSize = QSize(w, h);
            safeDeleteArray(d->frameData);
            d->frameData = new uchar[cbBitmapData];
        }
        data = d->frameData;
        uchar* dst = const_cast<uchar*>(data);
        const uchar* const dstEnd = dst + cbBitmapData;
        LONG stride = (LONG)MFGetAttributeUINT32(d->mediaType, MF_MT_DEFAULT_STRIDE, 1);
        bool upsideDown = (stride < 0);
        if (upsideDown) {
            stride = -stride;
            if (stride != w*3)
                goto done;
            for (int scanLine = h-1; scanLine >= 0; --scanLine) {
                const uchar* src = pBitmapData + scanLine * stride;
                const uchar* const srcEnd = src + w * 3;
                while (src < srcEnd) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                }
            }
        }
        else {
            const uchar* src = pBitmapData;
            while (dst < dstEnd) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            }
        }
    }
    pBuffer->Unlock();

done:
    SafeRelease(&pBuffer);
    SafeRelease(&pSample);
#endif
}

bool VideoCaptureDevice::requestFrameSize(const QSize& requestedSize)
{
#if defined(WIN32)
    if (d_ptr->mediaType == NULL) {
        qWarning() << "mediaType is null";
        return false;
    }
    HRESULT hr = MFSetAttributeSize(d_ptr->mediaType, MF_MT_FRAME_SIZE, (UINT32)requestedSize.width(), (UINT32)requestedSize.height());
    return SUCCEEDED(hr);
#else
#endif
    return false;
}

bool VideoCaptureDevice::startup(void)
{
    startedUp = (MFStartup(MF_VERSION) == S_OK);
    return startedUp;
}

QSize VideoCaptureDevice::frameSize(void) const
{
    return d_ptr->lastFrame.size();
}

QStringList VideoCaptureDevice::enumerate(void)
{
    if (!startedUp)
        startup();
    IMFActivate** dev = NULL;
    IMFAttributes* devAttr = NULL;
    UINT32 nDev = 0;
    QStringList devices;
    HRESULT hr = MFCreateAttributes(&devAttr, 1);
    if (SUCCEEDED(hr))
        hr = devAttr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (SUCCEEDED(hr))
        hr = MFEnumDeviceSources(devAttr, &dev, &nDev);
    if (SUCCEEDED(hr)) {
        for (UINT32 i = 0; i < nDev; ++i) {
            WCHAR* szFriendlyName = NULL;
            hr = dev[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, NULL);
            if (SUCCEEDED(hr)) {
                devices.append(QString::fromWCharArray(szFriendlyName));
                CoTaskMemFree(szFriendlyName);
            }
        }
    }
    if (FAILED(hr))
        qWarning() << "Cannot create the video capture device";
    SafeRelease(&devAttr);
    for (UINT32 i = 0; i < nDev; i++)
        SafeRelease(&dev[i]);
    CoTaskMemFree(dev);
    return devices;
}
