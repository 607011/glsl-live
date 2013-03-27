// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __DOUBLESLIDER_H_
#define __DOUBLESLIDER_H_

#include <QObject>
#include <QSlider>
#include <QtCore/QDebug>

class DoubleSlider : public QSlider
{
    Q_OBJECT

public:
    explicit DoubleSlider(double minV, double maxV, Qt::Orientation orientation, QWidget* parent = NULL)
        : QSlider(orientation, parent)
        , mMin(minV)
        , mRange(maxV - minV)
    {
        setMinimum(0);
        setMaximum(Divisor);
        QObject::connect(this, SIGNAL(valueChanged(int)), SLOT(divide(int)));
    }

    static const int Divisor = 100000;

public slots:
    void setDoubleValue(double v)
    {
        int i = int((v - mMin) / mRange * Divisor);
        setValue(i);
    }

private slots:
    void divide(int i)
    {
        double d = mMin + mRange * double(i) / Divisor;
        emit valueChanged(d);
    }

signals:
    void valueChanged(double);

private:
    double mMin;
    double mRange;
};


#endif // __DOUBLESLIDER_H_
