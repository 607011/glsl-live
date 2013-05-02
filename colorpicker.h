// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __COLORPICKER_H_
#define __COLORPICKER_H_

#include <QWidget>
#include <QColor>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSize>
#include <QScopedPointer>

class ColorPickerPrivate;

class ColorPicker : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPicker(const QString& name, QWidget* parent = NULL);
    ~ColorPicker();
    QSize minimumSizeHint(void) const { return QSize(16, 16); }
    QSize sizeHint(void) const { return QSize(16, 16); }

protected:
    void paintEvent(QPaintEvent*);
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void mousePressEvent(QMouseEvent*);

signals:
    void currentColorChanged(QColor);
    void colorSelected(QColor);
    void accepted(void);
    void rejected(void);

public slots:
    void setColor(const QColor&);
    
private:
    QScopedPointer<ColorPickerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ColorPicker)
    Q_DISABLE_COPY(ColorPicker)

};

#endif // __COLORPICKER_H_
