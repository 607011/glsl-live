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


class ChannelWidgetPrivate
{
public:
    ChannelWidgetPrivate(void)
    {
        reset();
    }
    void reset(void) {
        type = ChannelWidget::None;
        image = QImage(":/images/checkered.png");
        filename = QString();
    }
    ChannelWidget::Type type;
    QImage image;
    QString filename;
    int index;
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
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    sp.setWidthForHeight(true);
    setSizePolicy(sp);
    setMaximumSize(sizeHint());
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));
}

ChannelWidget::~ChannelWidget()
{
    // ...
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
        QRectF r = (imageAspect > windowAspect)
                ? QRectF(1, (height() - height() / imageAspect) / 2, width() - 2, height() / imageAspect - 2)
                : QRectF((width() - width() * imageAspect) / 2, 1, width() * imageAspect - 2, height() - 2);
        p.drawImage(r, d->image);
        break;
    }
    case None:
        // fall-through
    default:
        p.setBrush(QBrush(d->image));
        p.setPen(Qt::transparent);
        p.drawRect(rect());
        break;
    }
    p.setBrush(Qt::transparent);
    p.setPen(QColor(18, 20, 16));
    p.drawRect(0, 0, width()-1, height()-1);
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
            QString filename = fileUrl.remove("file:///");
#else
            QString filename = fileUrl.remove("file://");
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
    menu.addAction(tr("Remove"));
    QAction* selectedItem = menu.exec(globalPos);
    if (selectedItem->text() == tr("Remove")) {
        emit imageDropped(d->index, QImage());
        d->reset();
        update();
    }
}

QString ChannelWidget::channelName(int index)
{
    return QString("uChannel%1").arg(index);
}

