// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __FFT_H_
#define __FFT_H_

#include <QtGlobal>
#include <QVector>
#include <qmath.h>

class FFTPrivate;

template <typename T>
class FFT
{
public:
    FFT(int maxSize = 2048)
    {
        Q_ASSERT_X((maxSize & (maxSize-1)) == 0, "FFT::FFT()", "maxSize must be a power of 2");
        mSize = maxSize;
        calcWTable();
    }

    void transform(T* data, int sz, bool inv = false)
    {
        int i;
        int j = 0;
        int sz2 = sz*2;
        T q;
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
            bit = sz;
            while ((bit & j) != 0) {
                j &= ~bit;
                bit >>= 1;
            }
            j |= bit;
        }
        int tabskip = sz << 1;
        const QVector<T>& wtab = (inv)? wtabi : wtabf;
        int skip1, skip2, ix, j2;
        T wr, wi, d1r, d1i, d2r, d2i, d2wr, d2wi;
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

    void transform(const QVector<T>&, bool inv = false)
    {
        transform(const_cast<T*>(data.data()), data.size()/2, inv);
    }

    int size(void) const
    {
        return mSize;
    }

private:
    void calcWTable(void)
    {
        wtabf.resize(mSize);
        wtabi.resize(mSize);
        for (int i = 0; i != mSize; i += 2) {
            const T theta = T(M_PI) * i / T(mSize);
            wtabf[i]   = qCos(theta);
            wtabf[i+1] = qSin(theta);
            wtabi[i]   = wtabf.at(i);
            wtabi[i+1] = -wtabf.at(i+1);
        }
    }

    QVector<T> wtabf;
    QVector<T> wtabi;
    int mSize;
};

#endif // __FFT_H_
