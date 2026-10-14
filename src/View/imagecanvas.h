#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QColor>
#include <functional>
#include <algorithm>
#include <cmath>

// 大图：以适应窗口为1倍，滚轮0.5～2倍，左键拖动。
class ImageCanvas final : public QGraphicsView
{
public:
    explicit ImageCanvas(QWidget *parent=nullptr) : QGraphicsView(parent)
    {
        scene_ = new QGraphicsScene(this);
        setScene(scene_);
        item_ = scene_->addPixmap(QPixmap());
        setRenderHint(QPainter::SmoothPixmapTransform);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setBackgroundBrush(QColor("#111827"));
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    std::function<void(double)> zoomChanged;
    void setImage(const QImage &image)
    {
        item_->setPixmap(QPixmap::fromImage(image));
        const QRectF rect=item_->boundingRect();
        scene_->setSceneRect(rect.adjusted(-rect.width(),-rect.height(),rect.width(),rect.height()));
        zoom_ = 1.0;
        fitBase();
    }
    void resetZoom() { zoom_=1.0; fitBase(); }
protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (item_->pixmap().isNull()) { event->ignore(); return; }
        const double steps=event->angleDelta().y()/120.0;
        const double next=std::max(0.5,std::min(2.0,zoom_*std::pow(1.15,steps)));
        scale(next/zoom_,next/zoom_);
        zoom_=next;
        if(zoomChanged) zoomChanged(zoom_);
        event->accept();
    }
    void resizeEvent(QResizeEvent *event) override
    {
        QGraphicsView::resizeEvent(event);
        fitBase();
    }
private:
    void fitBase()
    {
        if(item_->pixmap().isNull()) return;
        const QRectF rect=item_->boundingRect();
        const double base=std::min((viewport()->width()-8)/rect.width(),
                                   (viewport()->height()-8)/rect.height());
        resetTransform();
        scale(std::max(0.001,base)*zoom_,std::max(0.001,base)*zoom_);
        centerOn(item_);
        if(zoomChanged) zoomChanged(zoom_);
    }
    QGraphicsScene *scene_;
    QGraphicsPixmapItem *item_;
    double zoom_=1.0;
};
#endif
