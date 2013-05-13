// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __VIDEOCAPTUREDEVICE_H_
#define __VIDEOCAPTUREDEVICE_H_

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QStringList>

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
#include <mfidl.h>
#endif

class MediaInputPrivate;

class MediaInput : public QObject
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
        , public IMFAsyncCallback
#endif
{
    Q_OBJECT
public:
    explicit MediaInput(int id = -1, QObject* parent = NULL);
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    explicit MediaInput(const QString& filename, QObject* parent = NULL);

    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult);
    STDMETHODIMP GetParameters(DWORD*, DWORD*);
    HRESULT WaitForCompletion(DWORD msec);
#endif
    virtual ~MediaInput();

    enum InputType { None, Audio, Video };

#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    static const UINT WM_APP_PLAYER_EVENT = WM_APP + 1;    // wparam = IMFMediaEvent*
    enum PlayerState
    {
        Closed = 0,     // No session.
        Ready,          // Session was created, ready to open a file.
        OpenPending,    // Session is opening a file.
        Started,        // Session is playing a file.
        Paused,         // Session is paused.
        Stopped,        // Session is stopped (ready to play).
        Closing         // Application has closed the session, but is waiting for MESessionClosed.
    };

    HRESULT hr(void) const;
    // Playback
    HRESULT OpenURL(const QString& url); // HRESULT OpenURL(PCWSTR sURL);
    HRESULT Play(void);
    HRESULT Pause(void);
    HRESULT Shutdown(void);
    HRESULT HandleEvent(UINT_PTR pUnkPtr);
    PlayerState GetState(void) const;
    // Video functionality
    HRESULT Repaint(void);
    HRESULT ResizeVideo(WORD width, WORD height);
    BOOL HasVideo(void) const;

    bool open(const QString& filename);
#endif
    bool open(int);
    bool isOpen(void) const;
    void close(void);
    const QImage& getLastFrame(void) const;
    const QImage& getCurrentFrame(void);
    void getRawFrame(const uchar*& data, int& length);
    void getRawFrame(const uchar*& data, int& w, int& h);
    QSize frameSize(void) const;
    bool setFrameSize(const QSize&);
    InputType type(void) const;

    static bool startup(void);
    static QStringList availableDevices(void);

protected:
#ifdef WITH_WINDOWS_MEDIA_FOUNDATION
    HRESULT Initialize(void);
    HRESULT CreateSession(void);
    HRESULT CloseSession(void);
    HRESULT StartPlayback(void);
    HRESULT CreateMediaSource(const QString& sURL); // HRESULT CreateMediaSource(PCWSTR sURL);
    HRESULT CreateTopologyFromSource(IMFTopology **ppTopology);
    HRESULT AddBranchToPartialTopology(
        IMFTopology *pTopology,
        IMFPresentationDescriptor *pSourcePD,
        DWORD iStream
        );
    // Media event handlers
    HRESULT OnTopologyReady(IMFMediaEvent* pEvent);
    HRESULT OnPresentationEnded(IMFMediaEvent* pEvent);
#endif

signals:
    
public slots:

private:
    QScopedPointer<MediaInputPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MediaInput)
    Q_DISABLE_COPY(MediaInput)

    static bool startedUp;
};

#endif // __VIDEOCAPTUREDEVICE_H_
