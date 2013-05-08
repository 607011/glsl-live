// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __FFT_H_
#define __FFT_H_

#include <QScopedPointer>

class FFTPrivate;

class FFT
{
public:
    FFT(int maxSz = 2048);
    ~FFT();
    void transform(qreal* data, bool inv);

private:
    static const double PI;
    void calcWTable(void);

    QScopedPointer<FFTPrivate> d_ptr;
    Q_DECLARE_PRIVATE(FFT)
    Q_DISABLE_COPY(FFT)
};

#endif // __FFT_H_
