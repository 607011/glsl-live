// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QVector>
#include <qmath.h>
#include "fft.h"

class FFTPrivate
{
public:
    QVector<qreal> wtabf;
    QVector<qreal> wtabi;
    int m_sz;
};

FFT::FFT(int maxSz)
    : d_ptr(new FFTPrivate)
{
    Q_D(FFT);
    d->m_sz = maxSz;
    Q_ASSERT((maxSz & (maxSz-1)) == 0);
    calcWTable();
}

FFT::~FFT()
{ /* ... */ }


const double FFT::PI = 3.14159265358979323846264338327950288;

void FFT::calcWTable(void)
{
    Q_D(FFT);
    d->wtabf.resize(d->m_sz);
    d->wtabi.resize(d->m_sz);
    for (int i = 0; i != d->m_sz; i += 2) {
        double th = PI * i / qreal(d->m_sz);
        d->wtabf[i  ] = qCos(th);
        d->wtabf[i+1] = qSin(th);
        d->wtabi[i  ] = d->wtabf.at(i);
        d->wtabi[i+1] = -d->wtabf.at(i+1);
    }
}

void FFT::transform(qreal* data, bool inv)
{
    Q_D(FFT);
    int i;
    int j = 0;
    int sz2 = d->m_sz*2;
    qreal q;
    int bit;
    for (i = 0; i < sz2; i += 2) {
        if (i > j) {
            q = data[i];
            data[i] = data[j];
            data[j] = q;
            q = data[i+1];
            data[i+1] = data[j+1];
            data[j+1] = q;
        }
        bit = d->m_sz;
        while ((bit & j) != 0) {
            j &= ~bit;
            bit >>= 1;
        }
        j |= bit;
    }
    int tabskip = d->m_sz << 1;
    const QVector<qreal>& wtab = (inv)? d->wtabi : d->wtabf;
    int skip1, skip2, ix, j2;
    double wr, wi, d1r, d1i, d2r, d2i, d2wr, d2wi;
    for (i = 0; i < sz2; i += 4) {
        d1r = data[i];
        d1i = data[i+1];
        d2r = data[i+2];
        d2i = data[i+3];
        data[i  ] = d1r+d2r;
        data[i+1] = d1i+d2i;
        data[i+2] = d1r-d2r;
        data[i+3] = d1i-d2i;
    }
    tabskip >>= 1;
    int imult = (inv) ? -1 : 1;
    for (i = 0; i < sz2; i += 8) {
        d1r = data[i];
        d1i = data[i+1];
        d2r = data[i+4];
        d2i = data[i+5];
        data[i  ] = d1r+d2r;
        data[i+1] = d1i+d2i;
        data[i+4] = d1r-d2r;
        data[i+5] = d1i-d2i;
        d1r = data[i+2];
        d1i = data[i+3];
        d2r = data[i+6]*imult;
        d2i = data[i+7]*imult;
        data[i+2] = d1r-d2i;
        data[i+3] = d1i+d2r;
        data[i+6] = d1r+d2i;
        data[i+7] = d1i-d2r;
    }
    tabskip >>= 1;
    for (skip1 = 16; skip1 <= sz2; skip1 <<= 1) {
        skip2 = skip1 >> 1;
        tabskip >>= 1;
        for (i = 0; i < sz2; i += skip1) {
            ix = 0;
            for (j = i; j != i+skip2; j += 2, ix += tabskip) {
                wr = wtab.at(ix);
                wi = wtab.at(ix+1);
                d1r = data[j];
                d1i = data[j+1];
                j2 = j+skip2;
                d2r = data[j2];
                d2i = data[j2+1];
                d2wr = d2r*wr - d2i*wi;
                d2wi = d2r*wi + d2i*wr;
                data[j]    = d1r+d2wr;
                data[j+1]  = d1i+d2wi;
                data[j2  ] = d1r-d2wr;
                data[j2+1] = d1i-d2wi;
            }
        }
    }
}
