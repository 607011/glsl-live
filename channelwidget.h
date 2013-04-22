// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __CHANNELWIDGET_H_
#define __CHANNELWIDGET_H_

#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include <QScopedPointer>

class ChannelWidgetPrivate;

class ChannelWidget : public QWidget
{
    Q_OBJECT
public:
    enum Type {
        Auto,
        Image,
        Video,
        Sound,
        Data2D,
        Data3D
    };

    explicit ChannelWidget(const QString& filename = QString(), Type = Auto, QWidget* parent = NULL);
    ~ChannelWidget();
    QSize minimumSizeHint(void) const { return QSize(40, 40); }
    QSize sizeHint(void) const { return QSize(80, 80); }

    int heightForWidth(int w) const { return w; }

    void load(const QString& filename, Type = Auto);
    
protected:
    void paintEvent(QPaintEvent*);

signals:
    
public slots:

private: // variables
    QScopedPointer<ChannelWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ChannelWidget)
    Q_DISABLE_COPY(ChannelWidget)
};

#endif // __CHANNELWIDGET_H_
