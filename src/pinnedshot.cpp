#include "pinnedshot.h"

#include <QMouseEvent>
#include <QPainter>

PinnedShot::PinnedShot(const QPixmap &pixmap, QWidget *parent)
    : QWidget(parent), pixmap_(pixmap) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(pixmap_.size());
}

void PinnedShot::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(0, 0, pixmap_);
    painter.setPen(QPen(QColor(220, 40, 40), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void PinnedShot::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void PinnedShot::mouseMoveEvent(QMouseEvent *event) {
    if (dragging_) {
        move(event->globalPosition().toPoint() - dragOffset_);
    }
}

void PinnedShot::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
    }
}

void PinnedShot::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        close();
    }
}
