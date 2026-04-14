#include "screenshotoverlay.h"

#include "selectiontoolbar.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cstdlib>

namespace {
constexpr QColor kShadeColor(0, 0, 0, 140);
constexpr int kResizeMargin = 8;
constexpr int kMinSelectionSize = 5;
constexpr int kMinAnnotationSize = 6;
}

ScreenshotOverlay::ScreenshotOverlay(QWidget *parent)
    : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    toolbar_ = new SelectionToolbar(this);
    connect(toolbar_, &SelectionToolbar::rectangleClicked, this, &ScreenshotOverlay::toggleRectangleMode);
    connect(toolbar_, &SelectionToolbar::undoClicked, this, &ScreenshotOverlay::undoLastAnnotation);
    connect(toolbar_, &SelectionToolbar::copyClicked, this, [this]() { copySelection(false); });
    connect(toolbar_, &SelectionToolbar::pinClicked, this, [this]() { copySelection(true); });
    connect(toolbar_, &SelectionToolbar::closeClicked, this, &ScreenshotOverlay::cancelCaptureInternal);
    hide();
}

void ScreenshotOverlay::beginCapture(const QPixmap &screenPixmap, const QRect &virtualGeometry) {
    screenPixmap_ = screenPixmap;
    virtualGeometry_ = virtualGeometry;
    selectionRect_ = QRect();
    annotationRect_ = QRect();
    selecting_ = false;
    drawingAnnotation_ = false;
    rectangleMode_ = false;
    draggingSelection_ = false;
    selectionAnchor_ = QPoint();
    dragOffset_ = QPoint();
    resizeAnchor_ = QPoint();
    annotationAnchor_ = QPoint();
    dragStartRect_ = QRect();
    annotationRects_.clear();
    selectionLocked_ = false;
    dragMode_ = DragMode::None;
    hideToolbar();
    setGeometry(virtualGeometry_);
    setCursor(Qt::CrossCursor);
    if (toolbar_) {
        toolbar_->setUndoEnabled(false);
    }
    show();
    raise();
    activateWindow();
}

void ScreenshotOverlay::resetState() {
    selectionRect_ = QRect();
    annotationRect_ = QRect();
    selecting_ = false;
    drawingAnnotation_ = false;
    rectangleMode_ = false;
    draggingSelection_ = false;
    selectionAnchor_ = QPoint();
    dragOffset_ = QPoint();
    resizeAnchor_ = QPoint();
    annotationAnchor_ = QPoint();
    dragStartRect_ = QRect();
    annotationRects_.clear();
    selectionLocked_ = false;
    dragMode_ = DragMode::None;
    screenPixmap_ = QPixmap();
    hideToolbar();
    setCursor(Qt::CrossCursor);
    hide();
}

void ScreenshotOverlay::paintEvent(QPaintEvent *) {
    if (screenPixmap_.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.drawPixmap(0, 0, screenPixmap_);
    painter.fillRect(rect(), kShadeColor);

    const QRect selection = normalizedSelection();
    if (!selection.isNull()) {
        painter.drawPixmap(selection.topLeft(), screenPixmap_.copy(selection));
        painter.setPen(QPen(Qt::white, 1));
        painter.drawRect(selection.adjusted(0, 0, -1, -1));
        drawAnnotationRectangles(painter);
    }
}

void ScreenshotOverlay::mousePressEvent(QMouseEvent *event) {
    if (selectionLocked_) {
        if (toolbar_ && toolbar_->isVisible() && toolbar_->geometry().contains(event->pos())) {
            QWidget::mousePressEvent(event);
            return;
        }
        if (event->button() != Qt::LeftButton) {
            return;
        }
        if (rectangleMode_) {
            if (!selectionRect_.contains(event->pos())) {
                return;
            }
            drawingAnnotation_ = true;
            annotationAnchor_ = event->pos();
            annotationRect_ = QRect(annotationAnchor_, annotationAnchor_);
            update();
            return;
        }
        dragMode_ = dragModeForPosition(event->pos());
        if (dragMode_ == DragMode::Move) {
            draggingSelection_ = true;
            dragOffset_ = event->pos() - selectionRect_.topLeft();
            dragStartRect_ = selectionRect_;
        } else if (dragMode_ != DragMode::None) {
            draggingSelection_ = true;
            resizeAnchor_ = event->pos();
            dragStartRect_ = selectionRect_;
        } else {
            cancelCaptureInternal();
        }
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    selecting_ = true;
    selectionAnchor_ = event->pos();
    selectionRect_ = QRect(selectionAnchor_, selectionAnchor_);
    update();
}

void ScreenshotOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (drawingAnnotation_) {
        QPoint currentPos = event->pos();
        currentPos.setX(std::clamp(currentPos.x(), selectionRect_.left(), selectionRect_.right()));
        currentPos.setY(std::clamp(currentPos.y(), selectionRect_.top(), selectionRect_.bottom()));
        annotationRect_ = QRect(annotationAnchor_, currentPos).normalized();
        update();
        return;
    }
    if (draggingSelection_) {
        if (dragMode_ == DragMode::Move) {
            selectionRect_ = clampedSelection(event->pos() - dragOffset_);
        } else {
            selectionRect_ = resizedSelection(event->pos());
        }
        showToolbar();
        update();
        return;
    }
    if (selectionLocked_) {
        updateCursor(event->pos());
    }
    if (!selecting_) {
        return;
    }
    selectionRect_ = QRect(selectionAnchor_, event->pos());
    update();
}

void ScreenshotOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (drawingAnnotation_) {
        drawingAnnotation_ = false;
        const QRect rect = normalizedAnnotationRect();
        if (!rect.isNull() && rect.width() >= kMinAnnotationSize && rect.height() >= kMinAnnotationSize) {
            annotationRects_.append(rect.translated(-selectionRect_.topLeft()));
            if (toolbar_) {
                toolbar_->setUndoEnabled(true);
            }
        }
        annotationRect_ = QRect();
        update();
        return;
    }
    if (draggingSelection_) {
        draggingSelection_ = false;
        dragMode_ = DragMode::None;
        showToolbar();
        updateCursor(event->pos());
        update();
        return;
    }
    if (!selecting_) {
        return;
    }
    selecting_ = false;
    selectionRect_ = QRect(selectionAnchor_, event->pos());
    finalizeSelection();
}

void ScreenshotOverlay::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (drawingAnnotation_) {
            drawingAnnotation_ = false;
            annotationRect_ = QRect();
            update();
            return;
        }
        if (rectangleMode_) {
            setRectangleMode(false);
            return;
        }
        cancelCaptureInternal();
        return;
    }
    if (event->key() == Qt::Key_F3 && hasSelection()) {
        copySelection(true);
        return;
    }
    QWidget::keyPressEvent(event);
}

QRect ScreenshotOverlay::normalizedSelection() const {
    if (selectionRect_.isNull()) {
        return QRect();
    }
    return selectionRect_.normalized();
}

QRect ScreenshotOverlay::clampedSelection(const QPoint &topLeft) const {
    if (selectionRect_.isNull()) {
        return {};
    }

    const int maxX = std::max(0, width() - selectionRect_.width());
    const int maxY = std::max(0, height() - selectionRect_.height());
    const int clampedX = std::clamp(topLeft.x(), 0, maxX);
    const int clampedY = std::clamp(topLeft.y(), 0, maxY);
    return QRect(QPoint(clampedX, clampedY), selectionRect_.size());
}

QRect ScreenshotOverlay::normalizedAnnotationRect() const {
    if (annotationRect_.isNull()) {
        return {};
    }
    return annotationRect_.normalized().intersected(selectionRect_);
}

QRect ScreenshotOverlay::resizedSelection(const QPoint &pos) const {
    QRect rect = dragStartRect_;
    const QPoint delta = pos - resizeAnchor_;

    switch (dragMode_) {
    case DragMode::ResizeTop:
        rect.setTop(rect.top() + delta.y());
        break;
    case DragMode::ResizeBottom:
        rect.setBottom(rect.bottom() + delta.y());
        break;
    case DragMode::ResizeLeft:
        rect.setLeft(rect.left() + delta.x());
        break;
    case DragMode::ResizeRight:
        rect.setRight(rect.right() + delta.x());
        break;
    case DragMode::ResizeTopLeft:
        rect.setTop(rect.top() + delta.y());
        rect.setLeft(rect.left() + delta.x());
        break;
    case DragMode::ResizeTopRight:
        rect.setTop(rect.top() + delta.y());
        rect.setRight(rect.right() + delta.x());
        break;
    case DragMode::ResizeBottomLeft:
        rect.setBottom(rect.bottom() + delta.y());
        rect.setLeft(rect.left() + delta.x());
        break;
    case DragMode::ResizeBottomRight:
        rect.setBottom(rect.bottom() + delta.y());
        rect.setRight(rect.right() + delta.x());
        break;
    case DragMode::None:
    case DragMode::Move:
        break;
    }

    if (rect.width() < kMinSelectionSize) {
        if (dragMode_ == DragMode::ResizeLeft || dragMode_ == DragMode::ResizeTopLeft || dragMode_ == DragMode::ResizeBottomLeft) {
            rect.setLeft(rect.right() - kMinSelectionSize + 1);
        } else if (dragMode_ != DragMode::ResizeTop && dragMode_ != DragMode::ResizeBottom) {
            rect.setRight(rect.left() + kMinSelectionSize - 1);
        }
    }

    if (rect.height() < kMinSelectionSize) {
        if (dragMode_ == DragMode::ResizeTop || dragMode_ == DragMode::ResizeTopLeft || dragMode_ == DragMode::ResizeTopRight) {
            rect.setTop(rect.bottom() - kMinSelectionSize + 1);
        } else if (dragMode_ != DragMode::ResizeLeft && dragMode_ != DragMode::ResizeRight) {
            rect.setBottom(rect.top() + kMinSelectionSize - 1);
        }
    }

    rect.setLeft(std::clamp(rect.left(), 0, width() - kMinSelectionSize));
    rect.setTop(std::clamp(rect.top(), 0, height() - kMinSelectionSize));
    rect.setRight(std::clamp(rect.right(), kMinSelectionSize - 1, width() - 1));
    rect.setBottom(std::clamp(rect.bottom(), kMinSelectionSize - 1, height() - 1));

    if (rect.width() < kMinSelectionSize) {
        rect.setWidth(kMinSelectionSize);
    }
    if (rect.height() < kMinSelectionSize) {
        rect.setHeight(kMinSelectionSize);
    }

    return rect;
}

ScreenshotOverlay::DragMode ScreenshotOverlay::dragModeForPosition(const QPoint &pos) const {
    if (!selectionRect_.adjusted(-kResizeMargin, -kResizeMargin, kResizeMargin, kResizeMargin).contains(pos)) {
        return DragMode::None;
    }

    const bool nearLeft = std::abs(pos.x() - selectionRect_.left()) <= kResizeMargin;
    const bool nearRight = std::abs(pos.x() - selectionRect_.right()) <= kResizeMargin;
    const bool nearTop = std::abs(pos.y() - selectionRect_.top()) <= kResizeMargin;
    const bool nearBottom = std::abs(pos.y() - selectionRect_.bottom()) <= kResizeMargin;

    if (nearLeft && nearTop) {
        return DragMode::ResizeTopLeft;
    }
    if (nearRight && nearTop) {
        return DragMode::ResizeTopRight;
    }
    if (nearLeft && nearBottom) {
        return DragMode::ResizeBottomLeft;
    }
    if (nearRight && nearBottom) {
        return DragMode::ResizeBottomRight;
    }
    if (nearTop && pos.x() >= selectionRect_.left() && pos.x() <= selectionRect_.right()) {
        return DragMode::ResizeTop;
    }
    if (nearBottom && pos.x() >= selectionRect_.left() && pos.x() <= selectionRect_.right()) {
        return DragMode::ResizeBottom;
    }
    if (nearLeft && pos.y() >= selectionRect_.top() && pos.y() <= selectionRect_.bottom()) {
        return DragMode::ResizeLeft;
    }
    if (nearRight && pos.y() >= selectionRect_.top() && pos.y() <= selectionRect_.bottom()) {
        return DragMode::ResizeRight;
    }
    if (selectionRect_.contains(pos)) {
        return DragMode::Move;
    }
    return DragMode::None;
}

Qt::CursorShape ScreenshotOverlay::cursorForDragMode(DragMode mode) const {
    switch (mode) {
    case DragMode::ResizeTop:
    case DragMode::ResizeBottom:
        return Qt::SizeVerCursor;
    case DragMode::ResizeLeft:
    case DragMode::ResizeRight:
        return Qt::SizeHorCursor;
    case DragMode::ResizeTopLeft:
    case DragMode::ResizeBottomRight:
        return Qt::SizeFDiagCursor;
    case DragMode::ResizeTopRight:
    case DragMode::ResizeBottomLeft:
        return Qt::SizeBDiagCursor;
    case DragMode::Move:
        return Qt::SizeAllCursor;
    case DragMode::None:
        return Qt::CrossCursor;
    }
    return Qt::CrossCursor;
}

void ScreenshotOverlay::updateCursor(const QPoint &pos) {
    if (rectangleMode_) {
        setCursor(selectionRect_.contains(pos) ? Qt::CrossCursor : Qt::ArrowCursor);
        return;
    }
    setCursor(cursorForDragMode(dragModeForPosition(pos)));
}

void ScreenshotOverlay::finalizeSelection() {
    QRect area = normalizedSelection();
    if (area.isNull() || area.width() < 5 || area.height() < 5) {
        cancelCaptureInternal();
        return;
    }

    selectionRect_ = area;
    selectionLocked_ = true;
    annotationRects_.clear();
    annotationRect_ = QRect();
    setCursor(Qt::SizeAllCursor);
    showToolbar();
    update();
}

void ScreenshotOverlay::cancelCaptureInternal() {
    resetState();
    emit captureCanceled();
}

void ScreenshotOverlay::dismiss() {
    resetState();
}

bool ScreenshotOverlay::hasSelection() const {
    return selectionLocked_ && !selectionRect_.isNull();
}

QPixmap ScreenshotOverlay::currentSelectionPixmap() const {
    if (!hasSelection()) {
        return {};
    }
    return renderedSelectionPixmap();
}

void ScreenshotOverlay::showToolbar() {
    if (!toolbar_) {
        return;
    }
    toolbar_->setRectangleMode(rectangleMode_);
    toolbar_->setUndoEnabled(!annotationRects_.isEmpty());
    toolbar_->showForSelection(selectionRect_, rect());
}

void ScreenshotOverlay::hideToolbar() {
    if (toolbar_) {
        toolbar_->hide();
    }
}

bool ScreenshotOverlay::copySelection(bool pin) {
    if (!hasSelection()) {
        return false;
    }
    QPixmap pix = currentSelectionPixmap();
    if (pix.isNull()) {
        return false;
    }
    emit captureConfirmed(pix);
    if (pin) {
        emit pinRequested(pix);
    }
    dismiss();
    return true;
}

void ScreenshotOverlay::toggleRectangleMode() {
    setRectangleMode(!rectangleMode_);
}

void ScreenshotOverlay::setRectangleMode(bool enabled) {
    rectangleMode_ = enabled;
    drawingAnnotation_ = false;
    annotationRect_ = QRect();
    if (toolbar_) {
        toolbar_->setRectangleMode(enabled);
    }
    setCursor(enabled ? Qt::CrossCursor : Qt::SizeAllCursor);
    update();
}

void ScreenshotOverlay::undoLastAnnotation() {
    if (annotationRects_.isEmpty()) {
        return;
    }
    annotationRects_.removeLast();
    if (toolbar_) {
        toolbar_->setUndoEnabled(!annotationRects_.isEmpty());
    }
    update();
}

void ScreenshotOverlay::drawAnnotationRectangles(QPainter &painter) const {
    auto drawRect = [&painter](const QRect &rect) {
        if (rect.isNull()) {
            return;
        }
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(239, 68, 68, 70), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
        painter.setPen(QPen(QColor(220, 38, 38, 230), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
        painter.restore();
    };

    for (const QRect &rect : annotationRects_) {
        drawRect(rect.translated(selectionRect_.topLeft()));
    }
    drawRect(normalizedAnnotationRect());
}

QPixmap ScreenshotOverlay::renderedSelectionPixmap() const {
    QPixmap pixmap = screenPixmap_.copy(selectionRect_);
    if (pixmap.isNull() || annotationRects_.isEmpty()) {
        return pixmap;
    }

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const QRect &rect : annotationRects_) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(239, 68, 68, 70), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
        painter.setPen(QPen(QColor(220, 38, 38, 230), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
    }
    painter.end();
    return pixmap;
}
