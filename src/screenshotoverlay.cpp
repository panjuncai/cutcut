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
}

ScreenshotOverlay::ScreenshotOverlay(QWidget *parent)
    : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    toolbar_ = new SelectionToolbar(this);
    connect(toolbar_, &SelectionToolbar::copyClicked, this, [this]() { copySelection(false); });
    connect(toolbar_, &SelectionToolbar::pinClicked, this, [this]() { copySelection(true); });
    connect(toolbar_, &SelectionToolbar::closeClicked, this, &ScreenshotOverlay::cancelCaptureInternal);
    hide();
}

void ScreenshotOverlay::beginCapture(const QPixmap &screenPixmap, const QRect &virtualGeometry) {
    screenPixmap_ = screenPixmap;
    virtualGeometry_ = virtualGeometry;
    selectionRect_ = QRect();
    selecting_ = false;
    draggingSelection_ = false;
    selectionAnchor_ = QPoint();
    dragOffset_ = QPoint();
    resizeAnchor_ = QPoint();
    dragStartRect_ = QRect();
    selectionLocked_ = false;
    dragMode_ = DragMode::None;
    hideToolbar();
    setGeometry(virtualGeometry_);
    setCursor(Qt::CrossCursor);
    show();
    raise();
    activateWindow();
}

void ScreenshotOverlay::resetState() {
    selectionRect_ = QRect();
    selecting_ = false;
    draggingSelection_ = false;
    selectionAnchor_ = QPoint();
    dragOffset_ = QPoint();
    resizeAnchor_ = QPoint();
    dragStartRect_ = QRect();
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
    return screenPixmap_.copy(selectionRect_);
}

void ScreenshotOverlay::showToolbar() {
    if (!toolbar_) {
        return;
    }
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
