// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include "kineticscroller.h"

#include <QMouseEvent>
#include <QScrollBar>
#include <QVector>
#include <QTime>
#include <QtCore/QtDebug>
#include <qmath.h>


class KineticScrollerPrivate {
public:
    KineticScrollerPrivate(QScrollArea* scrollArea)
        : scrollArea(scrollArea)
        , dragging(false)
        , timer(0)
    { /* ... */ }

    struct KineticData {
        KineticData(void) : t(0) { /* ... */ }
        KineticData(const QPoint& p, int t) : p(p), t(t) { /* ... */ }
        QPoint p;
        int t;
    };

    QScrollArea* scrollArea;
    bool dragging;
    QPoint lastMousePos;
    QVector<KineticData> kineticData;
    QTime mouseMoveTimer;
    int timer;
    QPointF velocity;

    static const double Friction;
    static const int TimeInterval;
    static const int NumKineticDataSamples;

};

const double KineticScrollerPrivate::Friction = 0.85;
const int KineticScrollerPrivate::TimeInterval = 20;
const int KineticScrollerPrivate::NumKineticDataSamples = 5;


KineticScroller::KineticScroller(QScrollArea* scrollArea, QObject* parent)
    : QObject(parent)
    , d_ptr(new KineticScrollerPrivate(scrollArea))
{
    Q_D(KineticScroller);
    if (d->scrollArea)
        attachTo(d->scrollArea);
}

KineticScroller::~KineticScroller()
{
    stopMotion();
    detach();
}

void KineticScroller::startMotion(const QPointF& velocity)
{
    Q_D(KineticScroller);
    d->velocity = velocity;
    if (d->timer == 0)
        d->timer = startTimer(KineticScrollerPrivate::TimeInterval);
}

void KineticScroller::stopMotion(void)
{
    Q_D(KineticScroller);
    if (d->timer) {
        killTimer(d->timer);
        d->timer = 0;
    }
    d->velocity = QPointF(0, 0);
}

void KineticScroller::detach(void)
{
    Q_D(KineticScroller);
    if (d->scrollArea)
        d->scrollArea->viewport()->removeEventFilter(this);
}

void KineticScroller::attachTo(QScrollArea* scrollArea)
{
    Q_D(KineticScroller);
    Q_ASSERT_X(d->scrollArea != NULL, "KineticScroller::attachTo()", "QScrollArea not given");
    detach();
    d->scrollArea = scrollArea;
    d->scrollArea->viewport()->installEventFilter(this);
}

bool KineticScroller::eventFilter(QObject* object, QEvent* event)
{
    Q_D(KineticScroller);
    Q_ASSERT_X(d->scrollArea != NULL, "KineticScroller::eventFilter()", "QScrollArea not set");
    Q_ASSERT_X(object == d->scrollArea->viewport(), "KineticScroller::eventFilter()", "Invalid QScrollArea");
    const QMouseEvent* const mouseEvent = reinterpret_cast<const QMouseEvent*>(event);
    bool doFilterEvent = true;
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (mouseEvent->button() == Qt::LeftButton) {
            stopMotion();
            d->scrollArea->viewport()->setCursor(Qt::ClosedHandCursor);
            d->lastMousePos = mouseEvent->pos();
            d->dragging = true;
            d->mouseMoveTimer.start();
            d->kineticData.clear();
        }
        break;
    case QEvent::MouseMove:
        if (d->dragging) {
            scrollBy(d->lastMousePos - mouseEvent->pos());
            d->kineticData.push_back(KineticScrollerPrivate::KineticData(mouseEvent->pos(), d->mouseMoveTimer.elapsed()));
            if (d->kineticData.count() > KineticScrollerPrivate::NumKineticDataSamples)
                d->kineticData.pop_front();
            d->lastMousePos = mouseEvent->pos();
        }
        break;
    case QEvent::MouseButtonRelease:
        if (d->dragging) {
            d->dragging = false;
            d->scrollArea->viewport()->setCursor(Qt::OpenHandCursor);
            if (d->kineticData.count() == KineticScrollerPrivate::NumKineticDataSamples) {
                const int timeSinceLastMoveEvent = d->mouseMoveTimer.elapsed() - d->kineticData.last().t;
                if (timeSinceLastMoveEvent < 100) {
                    const QPoint dp = d->kineticData.first().p - mouseEvent->pos();
                    const int dt = d->mouseMoveTimer.elapsed() - d->kineticData.first().t;
                    startMotion(1000 * dp / dt / KineticScrollerPrivate::TimeInterval);
                }
            }
        }
        break;
    case QEvent::Resize:
        emit sizeChanged(reinterpret_cast<const QResizeEvent*>(event)->size());
        break;
    default:
        doFilterEvent = false;
        break;
    }

    return doFilterEvent;
}

void KineticScroller::timerEvent(QTimerEvent*)
{
    Q_D(KineticScroller);
    if (d->velocity.manhattanLength() > M_SQRT2) {
        scrollBy(d->velocity.toPoint());
        d->velocity *= KineticScrollerPrivate::Friction;
    }
    else stopMotion();
}

void KineticScroller::scrollBy(const QPoint& offset)
{
    Q_D(KineticScroller);
    Q_ASSERT_X(d->scrollArea != NULL, "KineticScroller::scrollBy()", "QScrollArea not set");
    d->scrollArea->horizontalScrollBar()->setValue(d->scrollArea->horizontalScrollBar()->value() + offset.x());
    d->scrollArea->verticalScrollBar()->setValue(d->scrollArea->verticalScrollBar()->value() + offset.y());
}
