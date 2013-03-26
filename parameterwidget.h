// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __PARAMETERWIDGET_H_
#define __PARAMETERWIDGET_H_

#include <QWidget>
#include <QString>
#include <QVariant>

class ParameterWidget : public QWidget
{
    Q_OBJECT

public:
    enum Type {
        None = -1,
        Integer = 0,
        Float,
        Boolean
    };

    ParameterWidget(QWidget* parent = NULL);
    ParameterWidget(const ParameterWidget& o, QWidget* parent = NULL)
        : QWidget(parent)
        , mType(o.mType)
        , mName(o.mName)
        , mMinValue(o.mMinValue)
        , mMaxValue(o.mMaxValue)
        , mDefaultValue(o.mDefaultValue)
    { /* ... */ }

    void setType(Type t) { mType = t; }
    void setMinValue(int v) { mMinValue = v; }
    void setMinValue(float v) { mMinValue = v; }
    void setMaxValue(int v) { mMaxValue = v; }
    void setMaxValue(float v) { mMaxValue = v; }
    void setDefaultValue(int v) { mDefaultValue = v; }
    void setDefaultValue(float v) { mDefaultValue = v; }
    void setDefaultValue(bool v) { mDefaultValue = v; }

    Type type(void) const { return mType; }
    const QString& name(void) const { return mName; }
    QVariant minValue(void) const { return mMinValue; }
    QVariant maxValue(void) const { return mMaxValue; }
    QVariant defaultValue(void) const { return mDefaultValue; }

signals:

public slots:

private:
    Type mType;
    QString mName;
    QVariant mMinValue;
    QVariant mMaxValue;
    QVariant mDefaultValue;
};

#endif // __PARAMETERWIDGET_H_
