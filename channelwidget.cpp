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

#ifdef WITH_OPENCV
#include "webcam.h"
#include "webcamthread.h"
#endif


class ChannelWidgetPrivate
{
public:
    ChannelWidgetPrivate(void)
        : webcam(NULL)
        , webcamThread(NULL)
    {
        webcam = NULL;
        webcamThread = NULL;
        reset();
    }
    ~ChannelWidgetPrivate()
    {
//        safeDelete(webcam);
    }

    void reset(void)
    {
        type = ChannelWidget::None;
        image = QImage(":/images/checkered.png");
        filename = QString();
        camActive = false;
    }
    ChannelWidget::Type type;
    QImage image;
    QString filename;
    int index;
    bool camActive;

    Webcam* decoder(void)
    {
        if (webcam == NULL) {
            webcam = new Webcam;
            webcam->open(0);
        }
        return webcam;
    }

    void turnOffWebcam(void)
    {
        if (webcam->isOpen()) {
            webcam->close();
        }
    }

    WebcamThread* decoderThread(void)
    {
        if (webcamThread == NULL)
            webcamThread = new WebcamThread(decoder());
        return webcamThread;
    }

#ifdef WITH_OPENCV
    Webcam* webcam;
    WebcamThread* webcamThread;
#endif
};


ChannelWidget::ChannelWidget(int index, QWidget* parent)
    : QWidget(parent)
    , d_ptr(new ChannelWidgetPrivate)
{
    Q_D(ChannelWidget);
    d->index = index;
    const QString& name = channelName(index);
    setObjectName(name);
    setToolTip(name);
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    setMaximumSize(sizeHint());
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));

}

ChannelWidget::~ChannelWidget()
{
    // ...
}

void ChannelWidget::setImage(const QImage& img)
{
    Q_D(ChannelWidget);
    d->image = img;
    d->type = Image;
    update();
    emit imageDropped(d->index, d->image);
}

void ChannelWidget::load(const QString& filename, ChannelWidget::Type type)
{
    Q_D(ChannelWidget);
    if (filename.isEmpty())
        return;
    d->image.load(filename);
    d->type = type;
    setToolTip(filename);
    update();
}

int ChannelWidget::index(void) const
{
    return d_ptr->index;
}

void ChannelWidget::paintEvent(QPaintEvent*)
{
    Q_D(ChannelWidget);
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    switch (d->type) {
    case Auto:
        // fall-through
    case Image:
    {
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
        p.setBrush(QBrush(d->image));
        p.setPen(Qt::black);
        p.drawRect(0, 0, width()-1, height()-1);
        break;
    }
}

void ChannelWidget::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls() && e->mimeData()->urls().first().toString().contains(QRegExp("\\.(png|jpg|jpeg|gif|ico|mng|tga|tiff?)$", Qt::CaseInsensitive)))
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
        if (fileUrl.contains(QRegExp("file://.*\\.(png|jpg|jpeg|gif|ico|mng|tga|tiff?)$", Qt::CaseInsensitive))) {
#if defined(WIN32)
            const QString& filename = fileUrl.remove("file:///");
#else
            const QString& filename = fileUrl.remove("file://");
#endif
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
#ifdef WITH_OPENCV
    if (d->webcam == NULL)
        menu.addAction(tr("Use webcam"));
#endif
    QAction* selectedItem = menu.exec(globalPos);
    if (selectedItem == NULL)
        return;
    if (selectedItem->text() == tr("Remove")) {
        if (d->webcam != NULL && d->webcam->isOpen())
            d->turnOffWebcam();
        emit imageDropped(d->index, QImage());
        d->reset();
        update();
    }
    else if (selectedItem->text() == tr("Use webcam")) {
        connect(d->decoderThread(), SIGNAL(frameReady(QImage)), SLOT(setImage(QImage)));
        d->decoderThread()->startReading();
    }
}

QString ChannelWidget::channelName(int index)
{
    return QString("uChannel%1").arg(index);
}
