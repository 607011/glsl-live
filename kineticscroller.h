// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __KINETICSCROLLER_H_
#define __KINETICSCROLLER_H_

#include <QObject>
#include <QEvent>
#include <QScrollArea>
#include <QPoint>
#include <QTimerEvent>
#include <QScopedPointer>

class KineticScrollerPrivate;

class KineticScroller : public QObject
{
    Q_OBJECT
public:
    KineticScroller(QScrollArea* scrollArea = NULL, QObject* parent = NULL);
    ~KineticScroller();
    void attachTo(QScrollArea* scrollArea);
    void detach(void);

signals:
    void sizeChanged(QSize);

protected:
    bool eventFilter(QObject* object, QEvent* event);
    void timerEvent(QTimerEvent*);

private:
    void scrollBy(const QPoint&);
    void startMotion(const QPointF& velocity);
    void stopMotion(void);

    QScopedPointer<KineticScrollerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(KineticScroller)
    Q_DISABLE_COPY(KineticScroller)
};

#endif // __KINETICSCROLLER_H_
