#pragma once

#include <QPainter>
#include <QPixmap>
#include <QVector>
#include <QtMath>
#include <algorithm>

namespace ScreenImage {
struct Capture {
    QRect geometry; // Device-independent desktop coordinates.
    QPixmap pixmap; // Native pixels with the screen's device pixel ratio.
};

inline QPixmap compose(const QRect &geometry, const QVector<Capture> &captures) {
    if (geometry.isEmpty() || captures.isEmpty()) {
        return {};
    }
    qreal ratio = 1.0;
    for (const auto &capture : captures) {
        if (capture.pixmap.isNull()) {
            return {};
        }
        ratio = std::max(ratio, capture.pixmap.devicePixelRatio());
    }
    // Keep the densest screen's pixels rather than shrinking them to logical size.
    QPixmap canvas(qCeil(geometry.width() * ratio), qCeil(geometry.height() * ratio));
    canvas.setDevicePixelRatio(ratio);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    for (const auto &capture : captures) {
        const QRect target(capture.geometry.topLeft() - geometry.topLeft(), capture.geometry.size());
        painter.drawPixmap(QRectF(target), capture.pixmap, QRectF(capture.pixmap.rect()));
    }
    return canvas;
}

inline QPixmap crop(const QPixmap &source, const QRect &logicalRect) {
    if (source.isNull() || logicalRect.isEmpty()) {
        return {};
    }
    const qreal ratio = source.devicePixelRatio();
    // Map both edges independently so adjacent selections share pixel boundaries.
    const int left = qRound(logicalRect.x() * ratio);
    const int top = qRound(logicalRect.y() * ratio);
    const int right = qRound((logicalRect.x() + logicalRect.width()) * ratio);
    const int bottom = qRound((logicalRect.y() + logicalRect.height()) * ratio);
    const QRect pixels = QRect(left, top, right - left, bottom - top).intersected(source.rect());
    if (pixels.isEmpty()) {
        return {};
    }
    QPixmap result = source.copy(pixels);
    result.setDevicePixelRatio(ratio);
    return result;
}
}
