#ifndef __SIDEBARWIDGET_H_
#define __SIDEBARWIDGET_H_

#include <QVector>
#include <QColor>
#include <QPixmap>
#include <QFont>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWidget>
#include <QPlainTextEdit>

#include "glsl/glsledit.h"

struct BlockInfo {
    int position;
    int number;
    bool foldable;
    bool folded;
};

Q_DECLARE_TYPEINFO(BlockInfo, Q_PRIMITIVE_TYPE);

class SidebarWidget : public QWidget
{
public:
    SidebarWidget(AbstractEditor*);
    QVector<BlockInfo> lineNumbers;
    QColor backgroundColor;
    QColor lineNumberColor;
    QColor indicatorColor;
    QColor foldIndicatorColor;
    QFont font;
    int foldIndicatorWidth;
    QPixmap rightArrowIcon;
    QPixmap downArrowIcon;

protected:
    void mousePressEvent(QMouseEvent*);
    void paintEvent(QPaintEvent*);
};


#endif // __SIDEBARWIDGET_H_
