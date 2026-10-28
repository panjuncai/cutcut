#include "pinnedshot.h"

#include <QMouseEvent>
#include <QPainter>

namespace {
constexpr int kGlowMargin = 12;
}

PinnedShot::PinnedShot(const QPixmap &pixmap, QWidget *parent)
    : QWidget(parent), pixmap_(pixmap) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    resize(pixmap_.size() + QSize(kGlowMargin * 2, kGlowMargin * 2));
}

void PinnedShot::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRectF imageRect(QPointF(kGlowMargin, kGlowMargin), QSizeF(pixmap_.size()));
    // Paint the glow in transparent padding so it never covers the screenshot.
    painter.setBrush(Qt::NoBrush);
    for (int spread = kGlowMargin; spread > 0; --spread) {
        const qreal strength = 1.0 - qreal(spread) / (kGlowMargin + 1);
        const int alpha = qRound(35 * strength * strength);
        painter.setPen(QPen(QColor(128, 128, 128, alpha), 1));
        const qreal distance = spread - 0.5;
        painter.drawRoundedRect(imageRect.adjusted(-distance, -distance, distance, distance),
                                distance, distance);
    }
    painter.drawPixmap(kGlowMargin, kGlowMargin, pixmap_);
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
