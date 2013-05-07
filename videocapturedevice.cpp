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
        : hr(S_OK)
        , mediaPlayer(NULL)
        , mediaSource(NULL)
        , sourceReader(NULL)
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
    }

#if defined(WIN32)
    HRESULT hr;
    IMFPMediaPlayer* mediaPlayer;
    IMFMediaSource* mediaSource;
    IMFSourceReader* sourceReader;
    IMFActivate** camDevices;
    IMFAttributes* devAttr;
    UINT32 camCount;
#endif

    QImage lastFrame;


    void close(void)
    {
        SafeRelease(&sourceReader);
        SafeRelease(&mediaSource);
        SafeRelease(&mediaPlayer);
    }
};

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
    if (FAILED(hr))
        qWarning() << "Cannot create the video capture device";
    SafeRelease(&d->devAttr);
    if (deviceId <= int(d->camCount)) {
        d->camDevices[deviceId]->ActivateObject(__uuidof(IMFMediaSource),(void**)&d->mediaSource);
        qDebug() << "  activated" << d->mediaSource;
        hr = MFCreateSourceReaderFromMediaSource(d->mediaSource, NULL, &d->sourceReader);
        if (SUCCEEDED(hr)) {
            qDebug() << "  d->sourceReader =" << d->sourceReader;
        }
    }
    return hr == S_OK;
#else
    return false;
#endif
}

bool VideoCaptureDevice::isOpen(void) const
{
    return d_ptr->sourceReader != NULL;
}

void VideoCaptureDevice::close(void)
{
    Q_D(VideoCaptureDevice);
    d->close();
}

const QImage& VideoCaptureDevice::getLastFrame(void) const
{
    return d_ptr->lastFrame;
}

#if defined(WIN32)
inline float OffsetToFloat(const MFOffset& offset)
{
    return offset.value + (static_cast<float>(offset.fract) / 65536.0f);
}

RECT RectFromArea(const MFVideoArea& area)
{
    RECT rc;
    rc.left = static_cast<LONG>(OffsetToFloat(area.OffsetX));
    rc.top = static_cast<LONG>(OffsetToFloat(area.OffsetY));
    rc.right = rc.left + area.Area.cx;
    rc.bottom = rc.top + area.Area.cy;
    return rc;
}

void GetPixelAspectRatio(IMFMediaType *pType, MFRatio *pPar)
{
    HRESULT hr = MFGetAttributeRatio(pType,
        MF_MT_PIXEL_ASPECT_RATIO,
        (UINT32*)&pPar->Numerator,
        (UINT32*)&pPar->Denominator);
    if (FAILED(hr))
    {
        pPar->Numerator = pPar->Denominator = 1;
    }
}

struct FormatInfo
{
    UINT32          imageWidthPels;
    UINT32          imageHeightPels;
    BOOL            bTopDown;
    RECT            rcPicture;    // Corrected for pixel aspect ratio


    FormatInfo() : imageWidthPels(0), imageHeightPels(0), bTopDown(FALSE)
    {
        SetRectEmpty(&rcPicture);
    }
};
#endif


void VideoCaptureDevice::getRawFrame(const uchar*& data, int& w, int& h)
{
    Q_D(VideoCaptureDevice);
    IMFMediaBuffer* pBuffer = NULL;
    IMFSample* pSample = NULL;
    BYTE* pBitmapData = NULL;
    DWORD cbBitmapData = 0;
    UINT32 width = 0, height = 0;
    GUID subtype = { 0 };
    IMFMediaType* pType = NULL;
    data = NULL;
    w = -1;
    h = -1;
    DWORD dwFlags = 0;
    HRESULT hr = d->sourceReader->ReadSample(
                (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0,
                NULL,
                &dwFlags,
                NULL,
                &pSample
                );
    if (FAILED(hr))
        goto done;
    if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
        goto done;
    if (pSample == NULL)
        goto done;


    // Get the media type from the stream.
    hr = d->sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType);
    if (FAILED(hr))
        goto done;
    hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (subtype != MFVideoFormat_RGB24) {
        hr = E_UNEXPECTED;
        goto done;
    }
    hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED(hr))
        goto done;
    w = int(width);
    h = int(height);

    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr))
        goto done;
    hr = pBuffer->Lock(&pBitmapData, NULL, &cbBitmapData);
    if (FAILED(hr))
        goto done;
    pBuffer->Unlock();

done:
    SafeRelease(&pBuffer);
    SafeRelease(&pSample);
    qDebug() << cbBitmapData << w << h;
}

void VideoCaptureDevice::setSize(const QSize&)
{
    // TODO
}

bool VideoCaptureDevice::startup(void)
{
    return MFStartup(MF_VERSION) == S_OK;
}

QSize VideoCaptureDevice::frameSize(void) const
{
    return d_ptr->lastFrame.size();
}

QStringList VideoCaptureDevice::enumerate(void)
{
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
