// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QtCore/QDebug>
#include <QColorDialog>
#include <QCursor>
#include <QPainter>
#include "colorpicker.h"
#include "util.h"

class ColorPickerPrivate
{
public:
    ColorPickerPrivate(void)
        : colorDialog(new QColorDialog)
    { /* ... */ }
    ~ColorPickerPrivate()
    {
        safeDelete(colorDialog);
    }
    QCursor oldCursor;
    QColorDialog* colorDialog;
};

ColorPicker::ColorPicker(const QString& name, QWidget* parent)
    : QWidget(parent)
    , d_ptr(new ColorPickerPrivate)
{
    Q_D(ColorPicker);
    setObjectName(name);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    QObject::connect(d->colorDialog, SIGNAL(currentColorChanged(QColor)), SIGNAL(currentColorChanged(QColor)));
    QObject::connect(d->colorDialog, SIGNAL(colorSelected(QColor)), SLOT(setColor(QColor)));
    QObject::connect(d->colorDialog, SIGNAL(accepted()), SIGNAL(accepted()));
    QObject::connect(d->colorDialog, SIGNAL(rejected()), SIGNAL(rejected()));
    setMinimumSize(80, 17);
    setMaximumSize(120, 17);
}

ColorPicker::~ColorPicker()
{
    /* ... */
}

void ColorPicker::paintEvent(QPaintEvent*)
{
    Q_D(ColorPicker);
    QPainter p(this);
    const QColor& color = d->colorDialog->currentColor();
    p.fillRect(QRect(0, 0, 16, 16), color);
    p.setPen(Qt::black);
    p.setBrush(Qt::transparent);
    p.drawRect(QRect(0, 0, 16, 16));
    p.drawText(QRectF(20, 1, 80, 18), QString("#%1%2%3")
               .arg(color.red(), 2, 16, QChar('0'))
               .arg(color.green(), 2, 16, QChar('0'))
               .arg(color.blue(), 2, 16, QChar('0')));
}

void ColorPicker::enterEvent(QEvent*)
{
    d_ptr->oldCursor = cursor();
    setCursor(Qt::PointingHandCursor);
}

void ColorPicker::leaveEvent(QEvent*)
{
    setCursor(d_ptr->oldCursor);
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    Q_D(ColorPicker);
    if (e->button() == Qt::LeftButton) {
        d->colorDialog->show();
        d->colorDialog->raise();
        d->colorDialog->activateWindow();
    }
}

void ColorPicker::setColor(const QColor& color)
{
    d_ptr->colorDialog->setCurrentColor(color);
    emit colorSelected(color);
    update();
}
