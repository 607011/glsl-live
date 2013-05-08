// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QRegExp>
#include <QImage>
#include "mediainput.h"
#include "util.h"

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
#include <windows.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mfidl.h>
#endif

#ifdef WITH_OPENCV
#include <opencv/cv.h>
#include <opencv/cv.hpp>
#include <opencv/highgui.h>
#endif

class MediaInputPrivate
{
public:
    MediaInputPrivate(void)
        : frameData(NULL)
        , frameLength(0)
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
        , hr(S_OK)
        , mediaType(NULL)
        , mediaPlayer(NULL)
        , mediaSource(NULL)
        , sourceReader(NULL)
        , camDevices(NULL)
        , devAttr(NULL)
        , camCount(0)
        , inputType(MediaInput::None)
#endif
#ifdef WITH_OPENCV
        , webcam(NULL)
#endif
    {
        /* ... */
    }

    ~MediaInputPrivate()
    {
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
        for (DWORD i = 0; i < camCount; i++)
            SafeRelease(&camDevices[i]);
        CoTaskMemFree(camDevices);
#endif
        safeDelete(frameData);
    }

    uchar* frameData;
    int frameLength;
    QSize frameSize;
    QImage lastFrame;

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    HRESULT hr;
    IMFMediaType* mediaType;
    IMFPMediaPlayer* mediaPlayer;
    IMFMediaSource* mediaSource;
    IMFSourceReader* sourceReader;
    IMFActivate** camDevices;
    IMFAttributes* devAttr;
    UINT32 camCount;
    MediaInput::InputType inputType;
#endif

#ifdef WITH_OPENCV
    cv::VideoCapture* webcam;
    cv::Mat frame;
#endif

    void closeWebcam(void)
    {
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
        SafeRelease(&sourceReader);
        SafeRelease(&mediaSource);
        SafeRelease(&mediaPlayer);
#endif
#ifdef WITH_OPENCV
        if (webcam != NULL && webcam->isOpened())
            webcam->release();
#endif
    }
};


bool MediaInput::startedUp = false;



MediaInput::MediaInput(int id, QObject* parent)
    : QObject(parent)
    , d_ptr(new MediaInputPrivate)
{
    if (id >= 0)
        open(id);
}

MediaInput::MediaInput(const QString& filename, QObject* parent)
    : QObject(parent)
    , d_ptr(new MediaInputPrivate)
{
    if (!filename.isEmpty())
        open(filename);
}

MediaInput::~MediaInput()
{
    /* ... */
}

bool MediaInput::open(int deviceId)
{
    Q_D(MediaInput);
    close();

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
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
    d->inputType = SUCCEEDED(hr)? Video : None;
    return SUCCEEDED(hr);
#endif

#ifdef WITH_OPENCV
    d->webcam = new cv::VideoCapture(deviceId);
    if (d->webcam->isOpened()) {
        cv::Mat frame;
        d->webcam->read(frame);
        d->frameSize = QSize((int)d->webcam->get(CV_CAP_PROP_FRAME_WIDTH), (int)d->webcam->get(CV_CAP_PROP_FRAME_HEIGHT));
        return !d->frameSize.isNull();
    }
    return false;
#endif
}

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
bool MediaInput::open(const QString& filename)
{
    Q_D(MediaInput);
    close();
    HRESULT hr;
    hr = MFCreateAttributes(&d->devAttr, 1);
    if (filename.contains(QRegExp("(wmv)$", Qt::CaseInsensitive))) {
        if (SUCCEEDED(hr))
            hr = d->devAttr->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        if (FAILED(hr)) {
            qWarning() << "Cannot create the video input";
            SafeRelease(&d->devAttr);
            return false;
        }
        hr = MFCreateSourceReaderFromURL((LPCWSTR)filename.utf16(), d->devAttr, &d->sourceReader);
        SafeRelease(&d->mediaType);
        hr = MFCreateMediaType(&d->mediaType);
        if (SUCCEEDED(hr))
            hr = d->mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (SUCCEEDED(hr))
            hr = d->mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
        if (SUCCEEDED(hr))
            hr = d->sourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, d->mediaType);
        if (SUCCEEDED(hr))
            hr = d->sourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        d->inputType = SUCCEEDED(hr)? Video : None;
    }
    else if (filename.contains(QRegExp("(mp3|wav)$", Qt::CaseInsensitive))) {
        if (SUCCEEDED(hr))
            hr = MFCreateSourceReaderFromURL((LPCWSTR)filename.utf16(), d->devAttr, &d->sourceReader);
        SafeRelease(&d->mediaType);
        hr = MFCreateMediaType(&d->mediaType);
        if (SUCCEEDED(hr))
            hr = d->mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr))
            hr = d->mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_MP3);
        if (SUCCEEDED(hr))
            hr = d->sourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, d->mediaType);
        if (SUCCEEDED(hr))
            hr = d->sourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        d->inputType = SUCCEEDED(hr)? Audio : None;
    }
    SafeRelease(&d->devAttr);
    SafeRelease(&d->mediaType);
    return SUCCEEDED(hr);
}
#endif

bool MediaInput::isOpen(void) const
{
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    return d_ptr->sourceReader != NULL;
#endif
#ifdef WITH_OPENCV
    return d_ptr->webcam->isOpened();
#endif
}

void MediaInput::close(void)
{
    Q_D(MediaInput);
    d->closeWebcam();
}

const QImage& MediaInput::getLastFrame(void) const
{
    return d_ptr->lastFrame;
}

const QImage& MediaInput::getCurrentFrame(void)
{
    Q_D(MediaInput);
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    switch (d->inputType)
    {
    case Video:
    {
        int w = -1, h = -1;
        const BYTE* data = NULL;
        int Tries = 10;
        while (data == NULL && --Tries)
            getRawFrame(data, w, h);
        if (data != NULL && w > 0 && h > 0) {
            if (QSize(w, h) != d->lastFrame.size())
                d->lastFrame = QImage(QSize(w, h), QImage::Format_ARGB32);
            BYTE* dst = const_cast<BYTE*>(d->lastFrame.constBits());
            const BYTE* src = const_cast<BYTE*>(data);
            const BYTE* const srcEnd = src + w * h * 3;
            while (src < srcEnd) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = 0xffU;
            }
        }
        break;
    }
    case Audio:
    {
        d->lastFrame = QImage();
        break;
    }
    }

#endif
#ifdef WITH_OPENCV
    d->webcam->read(d->frame);
    const int w = d->frame.cols;
    const int h = d->frame.rows;
    if (d->lastFrame.size() != QSize(w, h))
        d->lastFrame = QImage(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y) {
        const uchar* src = d->frame.ptr(y);
        uchar* dst = d->lastFrame.scanLine(y);
        const uchar* const dstEnd = dst + 4*w;
        while (dst < dstEnd) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = 0xffU;
        }
    }
#endif
    return getLastFrame();
}

void MediaInput::getRawFrame(const uchar*& data, int& length)
{
    Q_D(MediaInput);
    data = NULL;
    length = -1;
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    HRESULT hr;
    IMFMediaBuffer* buffer = NULL;
    IMFSample* sample = NULL;
    GUID subtype = { 0 };
    d->mediaType = NULL;
    DWORD dwFlags = 0x00000000U;

    BYTE* samples = NULL;
    DWORD nSamples = 0;
    hr = d->sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &sample);
    if (FAILED(hr) || (sample == NULL) || ((dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0))
        goto done;
    hr = d->sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &d->mediaType);
    if (FAILED(hr))
        goto done;
    hr = d->mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (subtype != MFAudioFormat_MP3)
        goto done;
    hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr))
        goto done;
    hr = buffer->Lock(&samples, NULL, &nSamples);
    if (FAILED(hr))
        goto done;
    if (nSamples > 0) {
        if (d->frameLength != nSamples) {
            d->frameLength = nSamples;
            safeDeleteArray(d->frameData);
            d->frameData = new uchar[nSamples];
        }
        length = d->frameLength;
        data = d->frameData;
        const uchar* src = samples;
        uchar* dst = const_cast<uchar*>(data);
        const uchar* const dstEnd = dst + nSamples;
        while (dst < dstEnd)
            *dst++ = *src++;
    }
    buffer->Unlock();

done:
    SafeRelease(&buffer);
    SafeRelease(&sample);
#endif
}

void MediaInput::getRawFrame(const uchar*& data, int& w, int& h)
{
    Q_D(MediaInput);
    data = NULL;
    w = -1;
    h = -1;
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    HRESULT hr;
    IMFMediaBuffer* buffer = NULL;
    IMFSample* sample = NULL;
    GUID subtype = { 0 };
    d->mediaType = NULL;
    DWORD dwFlags = 0x00000000U;
    BYTE* pixels = NULL;
    DWORD nPixels = 0;
    hr = d->sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &dwFlags, NULL, &sample);
    if (FAILED(hr) || (sample == NULL) || ((dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0))
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
    hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr))
        goto done;
    hr = buffer->Lock(&pixels, NULL, &nPixels);
    if (FAILED(hr))
        goto done;
    if (nPixels > 0 && w > 0 && h > 0) {
        if (d->frameSize != QSize(w, h)) {
            d->frameSize = QSize(w, h);
            safeDeleteArray(d->frameData);
            d->frameData = new uchar[nPixels];
        }
        data = d->frameData;
        uchar* dst = const_cast<uchar*>(data);
        const uchar* const dstEnd = dst + nPixels;
        LONG stride = (LONG)MFGetAttributeUINT32(d->mediaType, MF_MT_DEFAULT_STRIDE, 1);
        bool upsideDown = (stride < 0);
        if (upsideDown) {
            stride = -stride;
            if (stride != w*3)
                goto done;
            for (int scanLine = h-1; scanLine >= 0; --scanLine) {
                const uchar* src = pixels + scanLine * stride;
                const uchar* const srcEnd = src + w * 3;
                while (src < srcEnd) {
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                }
            }
        }
        else {
            const uchar* src = pixels;
            while (dst < dstEnd) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            }
        }
    }
    buffer->Unlock();

done:
    SafeRelease(&buffer);
    SafeRelease(&sample);
#endif

#ifdef WITH_OPENCV
    if (d->webcam && d->webcam->isOpened()) {
        d->webcam->read(d->frame);
        w = d->frame.cols;
        h = d->frame.rows;
        data = d->frame.data;
    }
#endif
}
bool MediaInput::setFrameSize(const QSize& requestedSize)
{
    Q_D(MediaInput);
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    if (d->mediaType == NULL) {
        qWarning() << "media type is null";
        return false;
    }
    HRESULT hr = MFSetAttributeSize(d->mediaType, MF_MT_FRAME_SIZE, (UINT32)requestedSize.width(), (UINT32)requestedSize.height());
    return SUCCEEDED(hr);
#endif
#ifdef WITH_OPENCV
    d->webcam->set(CV_CAP_PROP_FRAME_WIDTH, requestedSize.width());
    d->webcam->set(CV_CAP_PROP_FRAME_HEIGHT, requestedSize.height());
#endif
    return false;
}

MediaInput::InputType MediaInput::type(void) const
{
    return d_ptr->inputType;
}

bool MediaInput::startup(void)
{
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    startedUp = (MFStartup(MF_VERSION) == S_OK);
    return startedUp;
#else
    startedUp = true;
    return true;
#endif
}

QSize MediaInput::frameSize(void) const
{
    return d_ptr->lastFrame.size();
}

QStringList MediaInput::availableDevices(void)
{
    if (!startedUp)
        startup();
    QStringList devices;
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    IMFActivate** dev = NULL;
    IMFAttributes* devAttr = NULL;
    UINT32 nDev = 0;
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
#endif
#ifdef WITH_OPENCV
    qWarning() << tr("There still is no function built into OpenCV to discover the available video input devices (see https://code.ros.org/trac/opencv/ticket/935). Assuming that there is at least one webcam connected and calling it `%1`.").arg(tr("first webcam")).toUtf8().constData();
    devices << tr("first webcam");
#endif
    return devices;
}
