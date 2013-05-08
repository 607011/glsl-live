// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QPainter>
#include <QImage>
#include <QString>
#include <QContextMenuEvent>
#include <QSizePolicy>
#include <QMimeData>
#include <QRegExp>
#include <QMenu>
#include <QAction>
#include <QString>

#include "channelwidget.h"
#include "util.h"

#include "mediainput.h"
#include "mediainputthread.h"

class ChannelWidgetPrivate
{
public:
    ChannelWidgetPrivate(int idx)
        : index(idx)
        , type(ChannelWidget::None)
        , videoInput(NULL)
        , videoInputThread(NULL)
        , isPlaying(false)
        , reFileTypes(QRegExp("\\.(png|jpg|jpeg|gif|ico|mng|tga|tiff?|wmv|mp3)$", Qt::CaseInsensitive))
    { /* ... */ }

    ~ChannelWidgetPrivate()
    { /* ... */ }

    void reset(void)
    {
        type = ChannelWidget::None;
        image = QImage();
        filename = QString();
    }

    int index;
    ChannelWidget::Type type;
    QImage image;
    QString filename;
    MediaInput* videoInput;
    MediaInputThread* videoInputThread;
    bool isPlaying;
    QStringList webcamList;
    QRegExp reFileTypes;

    inline MediaInput* decoder(int camId)
    {
        if (videoInput == NULL) {
            videoInput = new MediaInput;
            videoInput->open(camId);
        }
        return videoInput;
    }

    inline MediaInputThread* decoderThread(int camId = -1)
    {
        if (videoInputThread == NULL)
            videoInputThread = new MediaInputThread(decoder(camId));
        return videoInputThread;
    }

    inline MediaInput* decoder(const QString& filename)
    {
        if (videoInput == NULL) {
            videoInput = new MediaInput;
            videoInput->open(filename);
        }
        return videoInput;
    }

    inline MediaInputThread* decoderThread(const QString& filename)
    {
        if (videoInputThread == NULL)
            videoInputThread = new MediaInputThread(decoder(filename));
        return videoInputThread;
    }

    void stopStream(void)
    {
        if (videoInputThread)
            videoInputThread->stopReading();
        if (isStreamOpen())
            videoInput->close();
        safeDelete(videoInput);
        safeDelete(videoInputThread);
    }

    void pauseStream(void)
    {
        qDebug() << "PAUSE";
    }

    void startStream(void)
    {
        qDebug() << "START";
    }

    inline bool isStreamOpen(void)
    {
        return videoInput != NULL && videoInput->isOpen();
    }

};


ChannelWidget::ChannelWidget(int index, QWidget* parent)
    : QWidget(parent)
    , d_ptr(new ChannelWidgetPrivate(index))
{
    const QString& name = channelName(index);
    setObjectName(name);
    setToolTip(name);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    setMaximumSize(sizeHint());
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));
}

ChannelWidget::~ChannelWidget()
{
    closeWebcam();
}

void ChannelWidget::closeWebcam(void)
{
    Q_D(ChannelWidget);
    if (d->isStreamOpen()) {
        QObject::disconnect(d->decoderThread(), SIGNAL(frameReady(QImage)),
                            this, SLOT(setImage(QImage)));
        QObject::disconnect(d->decoderThread(), SIGNAL(rawFrameReady(const uchar*, int, int, Project::SourceSelector)),
                            this, SLOT(relayFrame(const uchar*, int, int, Project::SourceSelector)));
        d->stopStream();
    }
}

void ChannelWidget::setImage(const QImage& img, Type type)
{
    Q_D(ChannelWidget);
    d->image = img;
    d->type = type;
    update();
}

void ChannelWidget::relayFrame(const uchar* data, int w, int h, Project::SourceSelector source)
{
    emit rawFrameReady(data, w, h, d_ptr->index, source);
}

void ChannelWidget::load(const QString& filename, ChannelWidget::Type type)
{
    Q_D(ChannelWidget);
    if (filename.isEmpty())
        return;
    clear();
    d->image.load(filename);
    d->type = type;
    setToolTip(filename);
    update();
}

void ChannelWidget::stream(const QString& filename, ChannelWidget::Type type)
{
    Q_D(ChannelWidget);
    if (filename.isEmpty())
        return;
    clear();
    d->type = type;
    setToolTip(filename);
    QObject::connect(d->decoderThread(filename), SIGNAL(frameReady(QImage)),
                     SLOT(setImage(QImage)));
    QObject::connect(d->decoderThread(filename), SIGNAL(rawFrameReady(const uchar*, int, int, Project::SourceSelector)),
                     SLOT(relayFrame(const uchar*, int, int, Project::SourceSelector)));
    if (d->isStreamOpen()) {
        setImage(d->videoInput->getCurrentFrame(), type);
        emit camInitialized(d->index);
    }
    d->decoderThread()->startReading();
}

int ChannelWidget::index(void) const
{
    return d_ptr->index;
}

void ChannelWidget::clear(void)
{
    Q_D(ChannelWidget);
    closeWebcam();
    d->reset();
    emit imageDropped(d->index, QImage());
    update();
}

void ChannelWidget::paintEvent(QPaintEvent*)
{
    Q_D(ChannelWidget);
    QPainter p(this);
    switch (d->type) {
    case Auto:
        // fall-through
    case Video:
        // fall-through
    case Image:
    {
        p.fillRect(rect(), Qt::black);
        const qreal imageAspect = qreal(d->image.width()) / d->image.height();
        const qreal windowAspect = qreal(width()) / height();
        QRect r = (imageAspect > windowAspect)
                ? QRect(1, (height() - height() / imageAspect) / 2,
                        width() - 2, height() / imageAspect - 2)
                : QRect(1 + (width() - width() * imageAspect) / 2, 1,
                        width() * imageAspect - 2, height() - 2);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawImage(r, d->image);
        break;
    }
    case None:
        // fall-through
    default:
        p.setBrush(QBrush(QImage(":/images/checkered.png")));
        p.setPen(Qt::black);
        p.drawRect(0, 0, width()-1, height()-1);
        break;
    }
}

void ChannelWidget::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls() && e->mimeData()->urls().first().toString().contains(d_ptr->reFileTypes))
        e->acceptProposedAction();
    else
        e->ignore();
}

void ChannelWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

void ChannelWidget::dropEvent(QDropEvent* e)
{
    Q_D(ChannelWidget);
    if (e->mimeData()->hasUrls()) {
        QString fileUrl = e->mimeData()->urls().first().toString();
        if (fileUrl.contains(d_ptr->reFileTypes)) {
#ifdef Q_OS_WIN32
            const QString& filename = fileUrl.remove("file:///");
#else
            const QString& filename = fileUrl.remove("file://");
#endif
            if (fileUrl.contains(QRegExp("(wmv)$")))
                stream(filename, Video);
            else if (fileUrl.contains(QRegExp("(mp3|wav)$")))
                stream(filename, Audio);
            else
                load(filename);
            emit imageDropped(d->index, d->image);
        }
    }
}

void ChannelWidget::showContextMenu(const QPoint& p)
{
    Q_D(ChannelWidget);
    const QPoint& globalPos = mapToGlobal(p);
    QMenu menu;
    if (!d->image.isNull())
        menu.addAction(tr("Remove"));
    int i = 0;
    QStringListIterator cam(d->webcamList);
    while (cam.hasNext()) {
        QAction* action = new QAction(tr("Use %1").arg(cam.next()), NULL);
        action->setData(i++);
        menu.addAction(action);
    }
    if ((d->type == Video || d->type == Audio) && d->isStreamOpen()) {
        if (d->isPlaying) {
            menu.addAction(tr("Pause"));
            menu.addAction(tr("Stop"));
        }
        else {
            menu.addAction(tr("Play"));
        }
    }
    QAction* selectedItem = menu.exec(globalPos);
    if (selectedItem == NULL)
        return;
    if (selectedItem->text() == tr("Remove")) {
        clear();
    }
    else if (selectedItem->text().contains(tr("Use"))) {
        int camId = selectedItem->data().toInt();
        QObject::connect(d->decoderThread(camId), SIGNAL(frameReady(QImage)),
                         SLOT(setImage(QImage)));
        QObject::connect(d->decoderThread(camId), SIGNAL(rawFrameReady(const uchar*, int, int, Project::SourceSelector)),
                         SLOT(relayFrame(const uchar*, int, int, Project::SourceSelector)));
        if (d->isStreamOpen()) {
            setImage(d->videoInput->getCurrentFrame(), Video);
            emit camInitialized(d->index);
        }
        d->decoderThread()->startReading();
    }
}

void ChannelWidget::setAvailableWebcams(const QStringList& camlist)
{
    Q_D(ChannelWidget);
    d->webcamList = camlist;
}

QString ChannelWidget::channelName(int index)
{
    return QString("uChannel%1").arg(index);
}

