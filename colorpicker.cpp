// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <QPainter>
#include <QtCore/QDebug>
#include "colorpicker.h"
#include "util.h"

ColorPicker::ColorPicker(const QString& name, QWidget* parent)
    : QWidget(parent)
    , mColorDialog(new QColorDialog)
{
    setObjectName(name);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    connect(mColorDialog, SIGNAL(currentColorChanged(QColor)), SIGNAL(currentColorChanged(QColor)));
    connect(mColorDialog, SIGNAL(colorSelected(QColor)), SIGNAL(colorSelected(QColor)));
    connect(mColorDialog, SIGNAL(colorSelected(QColor)), SLOT(setColor(QColor)));
    setMinimumSize(80, 17);
    setMaximumSize(120, 17);
}

ColorPicker::~ColorPicker()
{
    safeDelete(mColorDialog);
}

void ColorPicker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const QColor& color = mColorDialog->currentColor();
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
    mOldCursor = cursor();
    setCursor(Qt::PointingHandCursor);
}

void ColorPicker::leaveEvent(QEvent*)
{
    setCursor(mOldCursor);
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        mColorDialog->show();
        mColorDialog->raise();
        mColorDialog->activateWindow();
    }
}

void ColorPicker::setColor(const QColor& color)
{
    mColorDialog->setCurrentColor(color);
    qDebug() << "ColorPicker::setColor(" << color << ")";
    emit colorSelected(color);
    update();
}
