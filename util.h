// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __UTIL_H_
#define __UTIL_H_

template <class T>
inline void safeDelete(T& a)
{
    if (a)
        delete a;
    a = 0;
}

template <class T>
inline void safeDeleteArray(T& a)
{
    if (a)
        delete [] a;
    a = 0;
}

template <class T>
inline void safeRenew(T& a, T obj)
{
    if (a)
        delete a;
    a = obj;
}


#if defined(WIN32)
template <class T>
void SafeRelease(T** ppT)
{
    if (*ppT != NULL) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}
#endif // defined(WIN32)


#endif // __UTIL_H_
