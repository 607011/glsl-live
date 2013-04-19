// Copyright (c) 2012-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifdef ENABLE_SCRIPTING

#include <QtScript/QScriptEngine>
#include "imageclass.h"


/*******************************************
 *
 * ImageClass
 *
 *******************************************/

Q_DECLARE_METATYPE(QImage*)
Q_DECLARE_METATYPE(ImageClass*)


ImageClass::ImageClass(QScriptEngine* engine)
    : QObject(engine)
    , QScriptClass(engine)
{
    qScriptRegisterMetaType<QImage>(engine, toScriptValue, fromScriptValue);
    mProto = engine->newQObject(new ImagePrototype(this),
                                QScriptEngine::QtOwnership,
                                QScriptEngine::SkipMethodsInEnumeration
                                | QScriptEngine::ExcludeSuperClassMethods
                                | QScriptEngine::ExcludeSuperClassProperties);
    mProto.setPrototype(engine->globalObject().property("Object").property("prototype"));
    mCtor = engine->newFunction(construct, mProto);
    mCtor.setData(engine->toScriptValue(this));
}

ImageClass::~ImageClass()
{
    /* ... */
}

QString ImageClass::name(void) const
{
    return QLatin1String("Image");
}

QScriptValue ImageClass::prototype(void) const
{
    return mProto;
}

QScriptValue ImageClass::constructor(void)
{
    return mCtor;
}

QScriptValue ImageClass::toScriptValue(QScriptEngine* eng, const QImage& img)
{
    const QScriptValue& ctor = eng->globalObject().property("Image");
    ImageClass* cls = qscriptvalue_cast<ImageClass*>(ctor.data());
    if (!cls)
        return eng->newVariant(QVariant::fromValue(img));
    return cls->newInstance(img);
}

void ImageClass::fromScriptValue(const QScriptValue& obj, QImage& img)
{
    img = qvariant_cast<QImage>(obj.data().toVariant());
}

QScriptValue ImageClass::newInstance(void)
{
    return newInstance(QImage());
}

QScriptValue ImageClass::newInstance(const QImage& img)
{
    const QScriptValue& data = engine()->newVariant(QVariant::fromValue(img));
    return engine()->newObject(this, data);
}

QScriptValue ImageClass::construct(QScriptContext* ctx, QScriptEngine*)
{
    ImageClass* cls = qscriptvalue_cast<ImageClass*>(ctx->callee().data());
    if (!cls)
        return QScriptValue();
    switch (ctx->argumentCount()) {
    case 0: // QImage()
        return cls->newInstance();
    case 1: // QImage(const QImage&) oder QImage(const QString& filename)
    {
        const QScriptValue& arg = ctx->argument(0);
        if (arg.instanceOf(ctx->callee())) // QImage(const QImage&)
            return cls->newInstance(qscriptvalue_cast<QImage>(arg));
        else if (arg.isString())
            return cls->newInstance(QImage(arg.toString())); // QImage(const QString&)
        break;
    }
    }
    return cls->newInstance();
}


/*******************************************
 *
 * ImagePrototype
 *
 *******************************************/

ImagePrototype::ImagePrototype(QObject* parent)
    : QObject(parent)
{
    /* ... */
}

ImagePrototype::~ImagePrototype()
{
    /* ... */
}

QSize ImagePrototype::size(void) const
{
    return thisImage()->size();
}

int ImagePrototype::width(void) const
{
    return thisImage()->size().width();
}

int ImagePrototype::height(void) const
{
    return thisImage()->size().height();
}

QImage* ImagePrototype::thisImage() const
{
    return qscriptvalue_cast<QImage*>(thisObject().data());
}

#endif
