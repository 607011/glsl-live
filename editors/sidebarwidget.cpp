#include "sidebarwidget.h"
#include <QPainter>

SidebarWidget::SidebarWidget(AbstractEditor* editor)
    : QWidget(editor)
    , foldIndicatorWidth(0)
{
    backgroundColor = Qt::lightGray;
    lineNumberColor = Qt::black;
    indicatorColor = Qt::white;
    foldIndicatorColor = Qt::lightGray;
}

void SidebarWidget::mousePressEvent(QMouseEvent* event)
{
    if (foldIndicatorWidth > 0) {
        int xofs = width() - foldIndicatorWidth;
        int lineNo = -1;
        int fh = fontMetrics().lineSpacing();
        int ys = event->pos().y();
        if (event->pos().x() > xofs) {
            foreach (BlockInfo ln, lineNumbers)
                if (ln.position < ys && (ln.position + fh) > ys) {
                    if (ln.foldable)
                        lineNo = ln.number;
                    break;
                }
        }
        if (lineNo >= 0) {
            AbstractEditor* editor = qobject_cast<AbstractEditor*>(parent());
            if (editor)
                editor->toggleFold(lineNo);
        }
    }
}

void SidebarWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.fillRect(event->rect(), backgroundColor);
    p.setPen(lineNumberColor);
    p.setFont(font);
    int fh = QFontMetrics(font).height();
    foreach (BlockInfo ln, lineNumbers)
        p.drawText(0, ln.position, width() - 4 - foldIndicatorWidth, fh, Qt::AlignRight, QString::number(ln.number));

    if (foldIndicatorWidth > 0) {
        int xofs = width() - foldIndicatorWidth;
        p.fillRect(xofs, 0, foldIndicatorWidth, height(), indicatorColor);

        // initialize (or recreate) the arrow icons whenever necessary
        if (foldIndicatorWidth != rightArrowIcon.width()) {
            QPainter iconPainter;
            QPolygonF polygon;

            int dim = foldIndicatorWidth;
            rightArrowIcon = QPixmap(dim, dim);
            rightArrowIcon.fill(Qt::transparent);
            downArrowIcon = rightArrowIcon;

            polygon << QPointF(dim * 0.4, dim * 0.25);
            polygon << QPointF(dim * 0.4, dim * 0.75);
            polygon << QPointF(dim * 0.8, dim * 0.5);
            iconPainter.begin(&rightArrowIcon);
            iconPainter.setRenderHint(QPainter::Antialiasing);
            iconPainter.setPen(Qt::NoPen);
            iconPainter.setBrush(foldIndicatorColor);
            iconPainter.drawPolygon(polygon);
            iconPainter.end();

            polygon.clear();
            polygon << QPointF(dim * 0.25, dim * 0.4);
            polygon << QPointF(dim * 0.75, dim * 0.4);
            polygon << QPointF(dim * 0.5, dim * 0.8);
            iconPainter.begin(&downArrowIcon);
            iconPainter.setRenderHint(QPainter::Antialiasing);
            iconPainter.setPen(Qt::NoPen);
            iconPainter.setBrush(foldIndicatorColor);
            iconPainter.drawPolygon(polygon);
            iconPainter.end();
        }

        foreach (BlockInfo ln, lineNumbers)
            if (ln.foldable) {
                if (ln.folded)
                    p.drawPixmap(xofs, ln.position, rightArrowIcon);
                else
                    p.drawPixmap(xofs, ln.position, downArrowIcon);
            }
    }
}


