#include "customgraphicsview.h"
#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QScrollBar>
#include <qmath.h>
#include <QPen>
#include <vector>
#include <iostream>

CustomGraphicsView::CustomGraphicsView(QWidget* parent) : QGraphicsView(parent), scene(this)
{
    setScene(&scene);
    fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void CustomGraphicsView::loadImage(QImage& image)
{
    scene.clear();
    line = nullptr;

    auto pixmap = QPixmap::fromImage(image);
    img = scene.addPixmap(pixmap);
    setSceneRect(-20, -20, image.width() + 20, image.height() + 20);

    scene.addRect(QRectF(-1, -1, image.width() + 1, image.height() + 1), QPen(QColor(0, 255, 0)));
}

void CustomGraphicsView::drawValidityRect(int x, int y, int w, int h, bool t)
{
    QGraphicsRectItem* r;
    r = new QGraphicsRectItem(x, y, w, h);
    if (t)
        r->setBrush(QColor(0, 255, 0, 70));
    else
        r->setBrush(QColor(255, 0, 0, 70));
    r->setPen(Qt::NoPen);
    scene.addItem(r);
}

void CustomGraphicsView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        pan = true;
        originX = e->x();
        originY = e->y();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    if (e->button() == Qt::LeftButton && enableLine)
    {
        begin = mapToScene(e->pos());
        begin.setX(static_cast<int>(begin.x()) + 0.5);
        begin.setY(static_cast<int>(begin.y()) + 0.5);
        e->accept();
    }
}

void CustomGraphicsView::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        pan = false;
        setCursor(Qt::ArrowCursor);
        e->accept();
        return;
    }
    if (e->button() == Qt::LeftButton && enableLine)
    {
        end = mapToScene(e->pos());
        end.setX(static_cast<int>(end.x()) + 0.5);
        end.setY(static_cast<int>(end.y()) + 0.5);
        e->accept();
        LineSampler();

        if (line)
        {
            line->setLine(QLineF(begin, end));
        }
        else
        {
            line = scene.addLine(QLineF(begin, end));
        }
        line->setPen(QPen(QColor(0, 255, 0)));
    }
}

void CustomGraphicsView::mouseMoveEvent(QMouseEvent* e)
{
    if (pan)
    {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (e->x() - originX));
        verticalScrollBar()->setValue(verticalScrollBar()->value() - (e->y() - originY));
        originX = e->x();
        originY = e->y();
        e->accept();
        return;
    }

    if (!enableLine)
        return;

    auto tmp = mapToScene(e->pos());
    tmp.setX(static_cast<int>(tmp.x()) + 0.5);
    tmp.setY(static_cast<int>(tmp.y()) + 0.5);
    if (line)
    {
        line->setLine(QLineF(begin, tmp));
    }
    else
    {
        line = scene.addLine(QLineF(begin, tmp));
    }
    line->setPen(QPen(QColor(0, 255, 0)));
}

void CustomGraphicsView::wheelEvent(QWheelEvent* event)
{
    const QPointF p0scene = mapToScene(event->pos());

    qreal factor = qPow(1.2, event->delta() / 240.0);
    scale(factor, factor);

    const QPointF p1mouse = mapFromScene(p0scene);
    const QPointF move = p1mouse - event->pos();   // The move
    horizontalScrollBar()->setValue(move.x() + horizontalScrollBar()->value());
    verticalScrollBar()->setValue(move.y() + verticalScrollBar()->value());
}

std::vector<int> CustomGraphicsView::bhm_line(int x1, int y1, int x2, int y2)
{
    std::vector<int> retval;
    auto Image = img->pixmap().toImage();

    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
    dx = x2 - x1;
    dy = y2 - y1;
    dx1 = abs(dx);
    dy1 = abs(dy);
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;
    if (dy1 <= dx1)
    {
        if (dx >= 0)
        {
            x = x1;
            y = y1;
            xe = x2;
        }
        else
        {
            x = x2;
            y = y2;
            xe = x1;
        }
        retval.push_back(qGray(Image.pixel(x, y)));
        for (i = 0; x < xe; i++)
        {
            x = x + 1;
            if (px < 0)
            {
                px = px + 2 * dy1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    y = y + 1;
                }
                else
                {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }
            retval.push_back(qGray(Image.pixel(x, y)));
        }
    }
    else
    {
        if (dy >= 0)
        {
            x = x1;
            y = y1;
            ye = y2;
        }
        else
        {
            x = x2;
            y = y2;
            ye = y1;
        }
        retval.push_back(qGray(Image.pixel(x, y)));
        for (i = 0; y < ye; i++)
        {
            y = y + 1;
            if (py <= 0)
            {
                py = py + 2 * dx1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    x = x + 1;
                }
                else
                {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }
            retval.push_back(qGray(Image.pixel(x, y)));
        }
    }
    return retval;
}

void CustomGraphicsView::LineSampler()
{
    int count = 0;
    count = LineSampler(begin.x(), begin.y(), end.x(), end.y());
    emit RIP(count);
}

int CustomGraphicsView::LineSampler(int x0, int y0, int x1, int y1)
{
    auto values = CustomGraphicsView::bhm_line(x0, y0, x1, y1);

    if (values.empty())
        return 0;

    int count = 0;

    int prev_state = 0;
    if (values[0] == 0)
    {
        count++;
    }
    else
    {
        prev_state = 255;
    }
    for (auto x : values)
    {
        if (x != prev_state && x == 0)
        {
            count++;
        }
        prev_state = x;
    }

    return count;
}

void CustomGraphicsView::DrawMaxLine(int x0, int y0, int x1, int y1)
{
    scene.addLine(x0, y0, x1, y1, QPen(QColor(0, 0, 255)));
}

bool CustomGraphicsView::getEnableLine() const
{
    return enableLine;
}

void CustomGraphicsView::setEnableLine(bool value)
{
    enableLine = value;
}
