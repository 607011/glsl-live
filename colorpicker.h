// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __COLORPICKER_H_
#define __COLORPICKER_H_

#include <QColorDialog>
#include <QWidget>
#include <QString>
#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QSize>

class ColorPicker : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPicker(const QString& name, QWidget* parent = NULL);
    ~ColorPicker();
    QSize minimumSizeHint(void) { return QSize(16, 16); }
    QSize sizeHint(void) { return QSize(16, 16); }

protected:
    void paintEvent(QPaintEvent*);
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);
    void mousePressEvent(QMouseEvent*);

signals:
    void currentColorChanged(QColor);
    void colorSelected(QColor);

public slots:
    void setColor(const QColor&);
    
private:
    QCursor mOldCursor;
    QColorDialog* mColorDialog;
};

#endif // __COLORPICKER_H_
