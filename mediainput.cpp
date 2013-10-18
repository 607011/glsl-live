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
#include <mferror.h>
#include <shlwapi.h>
#include <evr.h>
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
        , mediaSession(NULL)
        , mediaType(NULL)
        , mediaPlayer(NULL)
        , mediaSource(NULL)
        , sourceReader(NULL)
        , camDevices(NULL)
        , devAttr(NULL)
        , pStream(NULL)
        , pVideoDisplay(NULL)
        , camCount(0)
        , inputType(MediaInput::None)
        , cRef(1)
        , hrStatus(S_OK)
        , state(MediaInput::Closed)
        , hEvent(NULL)
        , hCloseEvent(NULL)
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
        SafeRelease(&pStream);
        CloseHandle(hEvent);
        CloseHandle(hCloseEvent);
#endif
        safeDelete(frameData);
    }

    uchar* frameData;
    int frameLength;
    QSize frameSize;
    QImage lastFrame;

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    HRESULT hr;
    IMFMediaSession* mediaSession;
    IMFMediaType* mediaType;
    IMFPMediaPlayer* mediaPlayer;
    IMFMediaSource* mediaSource;
    IMFSourceReader* sourceReader;
    IMFActivate** camDevices;
    IMFAttributes* devAttr;
    IMFByteStream* pStream;
    IMFVideoDisplayControl *pVideoDisplay;
    UINT32 camCount;
    MediaInput::InputType inputType;
    long cRef;
    HRESULT hrStatus;
    ULONG cbRead;
    HANDLE hEvent;
    HANDLE hCloseEvent;
    MediaInput::PlayerState state;
#endif

#ifdef WITH_OPENCV
    cv::VideoCapture* webcam;
    cv::Mat frame;
#endif

    void closeStream(void)
    {
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
        SafeRelease(&sourceReader);
        SafeRelease(&mediaSource);
        SafeRelease(&mediaPlayer);
        SafeRelease(&mediaType);
        SafeRelease(&mediaSession);
#endif
#ifdef WITH_OPENCV
        if (webcam != NULL && webcam->isOpened())
            webcam->release();
#endif
    }
};


bool MediaInput::startedUp = false;


#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
HRESULT CreateSourceStreamNode(
        IMFMediaSource *pSource,
        IMFPresentationDescriptor *pSourcePD,
        IMFStreamDescriptor *pSourceSD,
        IMFTopologyNode **ppNode
        );

HRESULT CreateOutputNode(
        IMFStreamDescriptor *pSourceSD,
        IMFTopologyNode **ppNode
        );
#endif


MediaInput::MediaInput(int id, QObject* parent)
    : QObject(parent)
    , d_ptr(new MediaInputPrivate)
{
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    Initialize();
    if (FAILED(d_ptr->hr))
        return;
#endif
    if (id >= 0)
        open(id);
}

MediaInput::MediaInput(const QString& filename, QObject* parent)
    : QObject(parent)
    , d_ptr(new MediaInputPrivate)
{
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    Initialize();
    if (FAILED(d_ptr->hr))
        return;
#endif
    if (!filename.isEmpty())
        open(filename);
}

MediaInput::~MediaInput()
{
    /* ... */
}

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
HRESULT MediaInput::hr(void) const
{
    return d_ptr->hr;
}
#endif

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
    if (filename.contains(QRegExp("(wmv|wma|asf|avi|mp4|m4v|mov)$", Qt::CaseInsensitive))) {
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
    else if (filename.contains(QRegExp("(mp3|wav|aac|adts)$", Qt::CaseInsensitive))) {
        if (SUCCEEDED(hr))
            hr = MFCreateSourceReaderFromURL((LPCWSTR)filename.utf16(), d->devAttr, &d->sourceReader);
        SafeRelease(&d->mediaType);
        hr = MFCreateMediaType(&d->mediaType);
        if (SUCCEEDED(hr))
            hr = d->mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr)) {
            if (filename.endsWith(".mp3")) {
                hr = d->mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_MP3);
            }
            else if (filename.endsWith(".wav")) {
                hr = d->mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
            }
        }
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
    d->closeStream();
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
    //    GUID subtype = { 0 };
    //    hr = d->mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
    //    if (subtype != MFAudioFormat_MP3)
    //        goto done;
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

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
HRESULT MediaInput::Initialize(void)
{
    Q_D(MediaInput);
    if (d->hCloseEvent)
        return MF_E_ALREADY_INITIALIZED;
    d->hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (d->hCloseEvent == NULL)
        d->hr = HRESULT_FROM_WIN32(GetLastError());
    return d->hr;
}

STDMETHODIMP MediaInput::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(MediaInput, IMFAsyncCallback),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) MediaInput::AddRef(void)
{
    Q_D(MediaInput);
    return InterlockedIncrement(&d->cRef);
}

STDMETHODIMP_(ULONG) MediaInput::Release(void)
{
    Q_D(MediaInput);
    long cRef = InterlockedDecrement(&d->cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

STDMETHODIMP MediaInput::GetParameters(DWORD*, DWORD*)
{
    return E_NOTIMPL;
}

HRESULT MediaInput::WaitForCompletion(DWORD msec)
{
    Q_D(MediaInput);
    DWORD result = WaitForSingleObject(d->hEvent, msec);
    switch (result)
    {
    case WAIT_TIMEOUT:
        return E_PENDING;
    case WAIT_ABANDONED:
        // fall-through
    case WAIT_OBJECT_0:
        return d->hrStatus;
    default:
        break;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT MediaInput::OpenURL(const QString& url)
{
    Q_D(MediaInput);
    IMFTopology *pTopology = NULL;
    // Create the media session.
    HRESULT hr = CreateSession();
    if (FAILED(hr))
        goto done;
    // Create the media source.
    hr = CreateMediaSource(url);
    if (FAILED(hr))
        goto done;
    // Create a partial topology.
    hr = CreateTopologyFromSource(&pTopology);
    if (FAILED(hr))
        goto done;
    // Set the topology on the media session.
    hr = d->mediaSession->SetTopology(0, pTopology);
    if (FAILED(hr))
        goto done;
    // Set our state to "open pending"
    d->state = OpenPending;
    // If SetTopology succeeds, the media session will queue an
    // MESessionTopologySet event.

done:
    if (FAILED(hr))
        d->state = Closed;
    SafeRelease(&pTopology);
    return hr;
}

HRESULT MediaInput::Play(void)
{
    Q_D(MediaInput);
    HRESULT hr;
    if (d->state != Paused && d->state != Stopped)
        return MF_E_INVALIDREQUEST;
    if (d->mediaSession == NULL || d->mediaSource == NULL)
        return E_UNEXPECTED;
    hr = StartPlayback();
    return hr;
}

HRESULT MediaInput::Pause(void)
{
    Q_D(MediaInput);
    HRESULT hr;
    if (d->state != Started)
        return MF_E_INVALIDREQUEST;
    if (d->mediaSession == NULL || d->mediaSource == NULL)
        return E_UNEXPECTED;
    hr = d->mediaSession->Pause();
    if (SUCCEEDED(hr))
        d->state = Paused;
    return hr;
}

HRESULT MediaInput::Repaint(void)
{
    Q_D(MediaInput);
    HRESULT hr = S_OK;
    if (d->pVideoDisplay)
        hr = d->pVideoDisplay->RepaintVideo();
    return hr;
}

HRESULT MediaInput::ResizeVideo(WORD width, WORD height)
{
    Q_D(MediaInput);
    HRESULT hr = S_OK;
    if (d->pVideoDisplay)
    {
        // Set the destination rectangle.
        // Leave the default source rectangle (0,0,1,1).
        RECT rcDest = { 0, 0, width, height };
        hr = d->pVideoDisplay->SetVideoPosition(NULL, &rcDest);
    }
    return hr;
}

STDMETHODIMP MediaInput::Invoke(IMFAsyncResult* pResult)
{
    Q_D(MediaInput);
    IMFMediaEvent* pEvent = NULL;

    // Get the event from the event queue.
    HRESULT hr = d->mediaSession->EndGetEvent(pResult, &pEvent);
    if (FAILED(hr))
        goto done;

    // Get the event type.
    MediaEventType meType;
    hr = pEvent->GetType(&meType);
    if (FAILED(hr))
        goto done;
    if (meType == MESessionClosed) {
        // If the session is closed, the application is waiting on the event
        // handle. Also, do not request any more events from the session.
        SetEvent(d->hCloseEvent);
    }
    else {
        // For all other events, ask the media session for the
        // next event in the queue.
        hr = d->mediaSession->BeginGetEvent(this, NULL);
        if (FAILED(hr))
            goto done;
    }

    // For most events, post the event as a private window message to the
    // application. This lets the application process the event on its main
    // thread.

    // However, if a call to IMFMediaSession::Close is pending, it means the
    // application is waiting on the m_hCloseEvent event handle. (Blocking
    // call.) In that case, we simply discard the event.

    // When IMFMediaSession::Close is called, MESessionClosed is NOT
    // necessarily the next event that we will receive. We may receive any
    // number of other events before receiving MESessionClosed.

    if (d->state != Closing) {
        // Leave a reference count on the event.
        pEvent->AddRef();
        // XXX: PostMessage(d->hWndEvent, WM_APP_PLAYER_EVENT, (WPARAM)pEvent, (LPARAM)0);
    }

done:
    SafeRelease(&pEvent);
    return S_OK;
}

HRESULT MediaInput::HandleEvent(UINT_PTR pUnkPtr)
{
    HRESULT hrStatus = S_OK;            // Event status
    MF_TOPOSTATUS TopoStatus = MF_TOPOSTATUS_INVALID;
    IMFMediaEvent *pEvent = NULL;

    // pUnkPtr is really an IUnknown pointer.
    IUnknown* pUnk = (IUnknown*)pUnkPtr;
    if (pUnk == NULL)
        return E_POINTER;

    HRESULT hr = pUnk->QueryInterface(IID_PPV_ARGS(&pEvent));
    if (FAILED(hr))
        goto done;

    // Get the event type.
    MediaEventType meType;
    hr = pEvent->GetType(&meType);
    if (FAILED(hr))
        goto done;

    // Get the event status. If the operation that triggered the event did
    // not succeed, the status is a failure code.
    hr = pEvent->GetStatus(&hrStatus);
    if (FAILED(hr))
        goto done;

    // Check if the async operation succeeded.
    if (FAILED(hrStatus)) {
        hr = hrStatus;
        goto done;
    }

    // Switch on the event type. Update the internal state of the CPlayer as needed.
    switch(meType)
    {
    case MESessionTopologyStatus:
        // Get the status code.
        hr = pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, (UINT32*)&TopoStatus);
        if (FAILED(hr))
            goto done;
        switch (TopoStatus)
        {
        case MF_TOPOSTATUS_READY:
            hr = OnTopologyReady(pEvent);
            break;
        default:
            // Nothing to do.
            break;
        }
        break;
    case MEEndOfPresentation:
        hr = OnPresentationEnded(pEvent);
        break;
    }

done:
    SafeRelease(&pUnk);
    SafeRelease(&pEvent);
    return hr;
}

HRESULT MediaInput::OnTopologyReady(IMFMediaEvent* pEvent)
{
    Q_D(MediaInput);
    Q_UNUSED(pEvent);
    SafeRelease(&d->pVideoDisplay);
    // Ask for the IMFVideoDisplayControl interface. This interface is
    // implemented by the EVR and is exposed by the media session as a service.
    // Note: This call is expected to fail if the source does not have video.
    MFGetService(d->mediaSession, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&d->pVideoDisplay));
    HRESULT hr = StartPlayback();
    return hr;
}

HRESULT MediaInput::OnPresentationEnded(IMFMediaEvent *pEvent)
{
    Q_D(MediaInput);
    Q_UNUSED(pEvent);
    // The session puts itself into the stopped state automatically.
    d->state = Stopped;
    return S_OK;
}

HRESULT MediaInput::CreateSession()
{
    Q_D(MediaInput);
    // Close the old session, if any.
    HRESULT hr = CloseSession();
    if (FAILED(hr))
        goto done;

    Q_ASSERT(d->state == Closed);

    // Create the media session.
    hr = MFCreateMediaSession(NULL, &d->mediaSession);
    if (FAILED(hr))
        goto done;

    d->state = Ready;

    // Start pulling events from the media session
    hr = d->mediaSession->BeginGetEvent((IMFAsyncCallback*)this, NULL);
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

HRESULT MediaInput::CloseSession(void)
{
    Q_D(MediaInput);
    HRESULT hr = S_OK;
    SafeRelease(&d->pVideoDisplay);

    // First close the media session.
    if (d->mediaSession)
    {
        d->state = Closing;
        hr = d->mediaSession->Close();
        if (FAILED(hr))
            goto done;
        // Wait for the close operation to complete
        WaitForSingleObject(d->hCloseEvent, 5000);
        // Now there will be no more events from this session.
    }

    // Complete shutdown operations.
    // Shut down the media source. (Synchronous operation, no events.)
    if (d->mediaSource)
        d->mediaSource->Shutdown();

    // Shut down the media session. (Synchronous operation, no events.)
    if (d->mediaSession)
        d->mediaSession->Shutdown();

    SafeRelease(&d->mediaSource);
    SafeRelease(&d->mediaSession);
    d->state = Closed;

done:
    return hr;
}

HRESULT MediaInput::StartPlayback()
{
    Q_D(MediaInput);
    Q_ASSERT(d->mediaSession != NULL);

    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    varStart.vt = VT_EMPTY;

    HRESULT hr = d->mediaSession->Start(&GUID_NULL, &varStart);
    if (SUCCEEDED(hr))
    {
        // Start is an asynchronous operation. However, we can treat our state
        // as being already started. If Start fails later, we'll get an
        // MESessionStarted event with an error code, and will update our state.
        d->state = Started;
    }
    PropVariantClear(&varStart);
    return hr;
}

HRESULT MediaInput::CreateMediaSource(const QString& url)
{
    Q_D(MediaInput);
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

    IMFSourceResolver* pSourceResolver = NULL;
    IUnknown* pSource = NULL;

    SafeRelease(&d->mediaSource);

    // Create the source resolver.
    HRESULT hr = MFCreateSourceResolver(&pSourceResolver);
    if (FAILED(hr))
        goto done;

    // Use the source resolver to create the media source.

    // Note: For simplicity this sample uses the synchronous method on
    // IMFSourceResolver to create the media source. However, creating a media
    // source can take a noticeable amount of time, especially for a network
    // source. For a more responsive UI, use the asynchronous
    // BeginCreateObjectFromURL method.

    hr = pSourceResolver->CreateObjectFromURL(
                (LPCWSTR)url.utf16(),                       // URL of the source.
                MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
                NULL,                       // Optional property store.
                &ObjectType,                // Receives the created object type.
                &pSource                    // Receives a pointer to the media source.
            );
    if (FAILED(hr))
        goto done;

    // Get the IMFMediaSource interface from the media source.
    hr = pSource->QueryInterface(IID_PPV_ARGS(&pSource));

done:
    SafeRelease(&pSourceResolver);
    SafeRelease(&pSource);
    return hr;
}

HRESULT MediaInput::CreateTopologyFromSource(IMFTopology **ppTopology)
{
    Q_D(MediaInput);
    Q_ASSERT(d->mediaSession != NULL);
    Q_ASSERT(d->mediaSource != NULL);
    IMFTopology *pTopology = NULL;
    IMFPresentationDescriptor* pSourcePD = NULL;
    DWORD cSourceStreams = 0;

    // Create a new topology.
    HRESULT hr = MFCreateTopology(&pTopology);
    if (FAILED(hr))
        goto done;

    // Create the presentation descriptor for the media source.
    hr = d->mediaSource->CreatePresentationDescriptor(&pSourcePD);
    if (FAILED(hr))
        goto done;

    // Get the number of streams in the media source.
    hr = pSourcePD->GetStreamDescriptorCount(&cSourceStreams);
    if (FAILED(hr))
        goto done;

    // For each stream, create the topology nodes and add them to the topology.
    for (DWORD i = 0; i < cSourceStreams; ++i) {
        hr = AddBranchToPartialTopology(pTopology, pSourcePD, i);
        if (FAILED(hr))
            goto done;
    }

    // Return the IMFTopology pointer to the caller.
    *ppTopology = pTopology;
    (*ppTopology)->AddRef();

done:
    SafeRelease(&pTopology);
    SafeRelease(&pSourcePD);
    return hr;
}

HRESULT MediaInput::AddBranchToPartialTopology(IMFTopology *pTopology, IMFPresentationDescriptor *pSourcePD, DWORD iStream)
{
    Q_D(MediaInput);
    Q_ASSERT(pTopology != NULL);
    IMFStreamDescriptor* pSourceSD = NULL;
    IMFTopologyNode* pSourceNode = NULL;
    IMFTopologyNode* pOutputNode = NULL;
    BOOL fSelected = FALSE;

    // Get the stream descriptor for this stream.
    HRESULT hr = pSourcePD->GetStreamDescriptorByIndex(iStream, &fSelected, &pSourceSD);
    if (FAILED(hr))
        goto done;

    // Create the topology branch only if the stream is selected.
    // Otherwise, do nothing.
    if (fSelected) {
        // Create a source node for this stream.
        hr = CreateSourceStreamNode(d->mediaSource, pSourcePD, pSourceSD, &pSourceNode);
        if (FAILED(hr))
            goto done;

        // Create the output node for the renderer.
        hr = CreateOutputNode(pSourceSD, &pOutputNode);
        if (FAILED(hr))
            goto done;

        // Add both nodes to the topology.
        hr = pTopology->AddNode(pSourceNode);
        if (FAILED(hr))
            goto done;

        hr = pTopology->AddNode(pOutputNode);
        if (FAILED(hr))
            goto done;

        // Connect the source node to the output node.
        hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
    }

done:
    // Clean up.
    SafeRelease(&pSourceSD);
    SafeRelease(&pSourceNode);
    SafeRelease(&pOutputNode);
    return hr;
}

HRESULT CreateSourceStreamNode(
    IMFMediaSource *pSource,
    IMFPresentationDescriptor *pSourcePD,
    IMFStreamDescriptor *pSourceSD,
    IMFTopologyNode **ppNode
    )
{
    if (!pSource || !pSourcePD || !pSourceSD || !ppNode)
        return E_POINTER;

    IMFTopologyNode *pNode = NULL;

    // Create the source-stream node.
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode);
    if (FAILED(hr))
        goto done;

    // Set attribute: Pointer to the media source.
    hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
    if (FAILED(hr))
        goto done;

    // Set attribute: Pointer to the presentation descriptor.
    hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pSourcePD);
    if (FAILED(hr))
        goto done;

    // Set attribute: Pointer to the stream descriptor.
    hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSourceSD);
    if (FAILED(hr))
        goto done;

    // Return the IMFTopologyNode pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}

HRESULT CreateOutputNode(
    IMFStreamDescriptor *pSourceSD,
    IMFTopologyNode **ppNode
    )
{

    IMFTopologyNode *pNode = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFActivate *pRendererActivate = NULL;

    GUID guidMajorType = GUID_NULL;

    // Get the stream ID.
    DWORD streamID = 0;
    pSourceSD->GetStreamIdentifier(&streamID); // Just for debugging, ignore any failures.

    // Get the media type handler for the stream.
    HRESULT hr = pSourceSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr))
        goto done;

    // Get the major media type.
    hr = pHandler->GetMajorType(&guidMajorType);
    if (FAILED(hr))
        goto done;

    // Create a downstream node.
    hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode);
    if (FAILED(hr))
        goto done;

    // Create an IMFActivate object for the renderer, based on the media type.
    if (MFMediaType_Audio == guidMajorType) {
        // Create the audio renderer.
        hr = MFCreateAudioRendererActivate(&pRendererActivate);
    }
    else if (MFMediaType_Video == guidMajorType) {
//        // Create the video renderer.
//        hr = MFCreateVideoRendererActivate(hwndVideo, &pRendererActivate);
    }
    else {
        hr = E_FAIL;
    }
    if (FAILED(hr))
        goto done;

    // Set the IActivate object on the output node.
    hr = pNode->SetObject(pRendererActivate);
    if (FAILED(hr))
        goto done;

    // Return the IMFTopologyNode pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    SafeRelease(&pHandler);
    SafeRelease(&pRendererActivate);
    return hr;
}


#endif
