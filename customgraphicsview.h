#ifndef CUSTOMGRAPHICSVIEW_H
#define CUSTOMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QImage>
#include <vector>

class CustomGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    CustomGraphicsView(QWidget* parent = nullptr);

    void loadImage(QImage& image);
    void drawValidityRect(int x, int y, int w, int h, bool t);

    bool getEnableLine() const;
    void setEnableLine(bool value);

public slots:
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void wheelEvent(QWheelEvent* event);

signals:
    void RIP(int);
private:
    std::vector<int> bhm_line(int x1, int y1, int x2, int y2);
    void LineSampler();

    QGraphicsScene scene;

    QPointF begin;
    QPointF end;
    QGraphicsLineItem* line = nullptr;
    QGraphicsPixmapItem* img = nullptr;

    int originX;
    int originY;
    bool pan;

    bool enableLine = false;
};

#endif   // CUSTOMGRAPHICSVIEW_H
