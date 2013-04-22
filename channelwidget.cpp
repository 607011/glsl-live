// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QPainter>
#include <QImage>
#include <QString>
#include <QContextMenuEvent>
#include <QSizePolicy>

#include "channelwidget.h"


class ChannelWidgetPrivate
{
public:
    ChannelWidget::Type type;
    QImage image;
    QString filename;
};


ChannelWidget::ChannelWidget(const QString& filename, Type type, QWidget* parent)
    : QWidget(parent)
    , d_ptr(new ChannelWidgetPrivate)
{
    Q_D(ChannelWidget);
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    sp.setWidthForHeight(true);
    setSizePolicy(sp);
    setMaximumSize(100, 100);
    d->filename = filename;
    d->type = type;
    if (!d->filename.isEmpty())
        load(d->filename, d->type);
}

ChannelWidget::~ChannelWidget()
{
    // ...
}

void ChannelWidget::load(const QString& filename, ChannelWidget::Type)
{
    Q_D(ChannelWidget);
    if (filename.isEmpty())
        return;
    d->image.load(filename);
    setToolTip(filename);
    update();
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
    default:
        break;
    }
    p.setBrush(Qt::transparent);
    p.setPen(QColor(18, 20, 16));
    p.drawRect(0, 0, width()-1, height()-1);
}
