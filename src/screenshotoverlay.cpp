#include "screenshotoverlay.h"

#include "selectiontoolbar.h"
#include "screenimage.h"

#include <QFontMetrics>
#include <QKeyEvent>
#include <QLineF>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr QColor kShadeColor(0, 0, 0, 140);
constexpr int kResizeMargin = 8;
constexpr int kMinSelectionSize = 5;
constexpr int kMinAnnotationSize = 6;
constexpr int kMosaicCellSize = 14;
constexpr double kPi = 3.14159265358979323846;

void drawMosaicRegion(QPainter &painter, const QPixmap &source, const QRect &targetRect) {
    if (source.isNull() || targetRect.isNull() || targetRect.width() < 2 || targetRect.height() < 2) {
        return;
    }

    const QPixmap region = ScreenImage::crop(source, targetRect);
    if (region.isNull()) {
        return;
    }

    const QSize scaledSize(std::max(1, targetRect.width() / kMosaicCellSize),
                           std::max(1, targetRect.height() / kMosaicCellSize));
    const QPixmap downscaled = region.scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    const QPixmap pixelated = downscaled.scaled(region.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
    painter.drawPixmap(QRectF(targetRect), pixelated, QRectF(pixelated.rect()));
}

void drawArrowShape(QPainter &painter, const QPoint &start, const QPoint &end, const QPoint &offset = QPoint()) {
    const QPointF p1 = start + offset;
    const QPointF p2 = end + offset;
    QLineF line(p1, p2);
    if (line.length() < 4.0) {
        return;
    }

    const double arrowSize = 12.0;
    const double angle = std::atan2(-line.dy(), line.dx());
    const QPointF arrowP1 = p2 - QPointF(std::cos(angle + kPi / 6) * arrowSize, -std::sin(angle + kPi / 6) * arrowSize);
    const QPointF arrowP2 = p2 - QPointF(std::cos(angle - kPi / 6) * arrowSize, -std::sin(angle - kPi / 6) * arrowSize);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(239, 68, 68, 70), 6.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(p1, p2);
    painter.drawLine(p2, arrowP1);
    painter.drawLine(p2, arrowP2);
    painter.setPen(QPen(QColor(220, 38, 38, 235), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(p1, p2);
    painter.drawLine(p2, arrowP1);
    painter.drawLine(p2, arrowP2);
    painter.restore();
}
}

ScreenshotOverlay::ScreenshotOverlay(QWidget *parent)
    : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    textEditor_ = new QLineEdit(this);
    textEditor_->hide();
    textEditor_->setFrame(false);
    textEditor_->setAttribute(Qt::WA_ShowWithoutActivating);
    textEditor_->setAttribute(Qt::WA_TranslucentBackground);
    textEditor_->setStyleSheet("QLineEdit {"
                               "background: transparent;"
                               "border: none;"
                               "padding: 0;"
                               "selection-background-color: rgba(59, 130, 246, 0.22);"
                               "}");
    connect(textEditor_, &QLineEdit::editingFinished, this, &ScreenshotOverlay::commitTextInput);
    toolbar_ = new SelectionToolbar(this);
    connect(toolbar_, &SelectionToolbar::rectangleClicked, this, &ScreenshotOverlay::toggleRectangleMode);
    connect(toolbar_, &SelectionToolbar::arrowClicked, this, &ScreenshotOverlay::toggleArrowMode);
    connect(toolbar_, &SelectionToolbar::textClicked, this, &ScreenshotOverlay::toggleTextMode);
    connect(toolbar_, &SelectionToolbar::mosaicClicked, this, &ScreenshotOverlay::toggleMosaicMode);
    connect(toolbar_, &SelectionToolbar::undoClicked, this, &ScreenshotOverlay::undoLastAnnotation);
    connect(toolbar_, &SelectionToolbar::copyClicked, this, [this]() { copySelection(false); });
    connect(toolbar_, &SelectionToolbar::pinClicked, this, [this]() { copySelection(true); });
    connect(toolbar_, &SelectionToolbar::closeClicked, this, &ScreenshotOverlay::cancelCaptureInternal);
    connect(toolbar_, &SelectionToolbar::textSizeChanged, this, [this](int size) { currentTextSize_ = size; });
    connect(toolbar_, &SelectionToolbar::textColorChanged, this, [this](const QColor &color) { currentTextColor_ = color; });
    hide();
}

void ScreenshotOverlay::beginCapture(const QPixmap &screenPixmap, const QRect &virtualGeometry) {
    screenPixmap_ = screenPixmap;
    virtualGeometry_ = virtualGeometry;
    selectionRect_ = QRect();
    annotationRect_ = QRect();
    arrowStart_ = QPoint();
    arrowEnd_ = QPoint();
    selecting_ = false;
    drawingAnnotation_ = false;
    rectangleMode_ = false;
    arrowMode_ = false;
    textMode_ = false;
    mosaicMode_ = false;
    draggingSelection_ = false;
    selectionAnchor_ = QPoint();
    dragOffset_ = QPoint();
    resizeAnchor_ = QPoint();
    annotationAnchor_ = QPoint();
    dragStartRect_ = QRect();
    annotationRects_.clear();
    arrowAnnotations_.clear();
    mosaicRects_.clear();
    textAnnotations_.clear();
    annotationHistory_.clear();
    selectionLocked_ = false;
    dragMode_ = DragMode::None;
    hideToolbar();
    setGeometry(virtualGeometry_);
    setCursor(Qt::CrossCursor);
    if (toolbar_) {
        toolbar_->setUndoEnabled(false);
        toolbar_->setRectangleMode(false);
        toolbar_->setArrowMode(false);
        toolbar_->setTextMode(false);
        toolbar_->setMosaicMode(false);
        toolbar_->setCurrentTextSize(currentTextSize_);
        toolbar_->setCurrentTextColor(currentTextColor_);
    }
    cancelTextInput();
    show();
    raise();
    activateWindow();
}

void ScreenshotOverlay::resetState() {
    selectionRect_ = QRect();
    annotationRect_ = QRect();
    arrowStart_ = QPoint();
    arrowEnd_ = QPoint();
    selecting_ = false;
    drawingAnnotation_ = false;
    rectangleMode_ = false;
    arrowMode_ = false;
    textMode_ = false;
    mosaicMode_ = false;
    draggingSelection_ = false;
    selectionAnchor_ = QPoint();
    dragOffset_ = QPoint();
    resizeAnchor_ = QPoint();
    annotationAnchor_ = QPoint();
    dragStartRect_ = QRect();
    annotationRects_.clear();
    arrowAnnotations_.clear();
    mosaicRects_.clear();
    textAnnotations_.clear();
    annotationHistory_.clear();
    selectionLocked_ = false;
    dragMode_ = DragMode::None;
    screenPixmap_ = QPixmap();
    hideToolbar();
    cancelTextInput();
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
        painter.save();
        painter.setClipRect(selection);
        painter.drawPixmap(0, 0, screenPixmap_);
        painter.restore();
        drawMosaicAnnotations(painter);
        painter.setPen(QPen(Qt::white, 1));
        painter.drawRect(selection.adjusted(0, 0, -1, -1));
        drawAnnotationRectangles(painter);
        drawArrowAnnotations(painter);
        drawTextAnnotations(painter);
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
        if (textMode_) {
            if (!selectionRect_.contains(event->pos())) {
                return;
            }
            beginTextInput(event->pos());
            return;
        }
        if (rectangleMode_ || mosaicMode_ || arrowMode_) {
            if (!selectionRect_.contains(event->pos())) {
                return;
            }
            drawingAnnotation_ = true;
            annotationAnchor_ = event->pos();
            annotationRect_ = QRect(annotationAnchor_, annotationAnchor_);
            arrowStart_ = event->pos();
            arrowEnd_ = event->pos();
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
        arrowEnd_ = currentPos;
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
        const QPoint localStart = arrowStart_ - selectionRect_.topLeft();
        const QPoint localEnd = arrowEnd_ - selectionRect_.topLeft();
        if (arrowMode_) {
            if (QLineF(localStart, localEnd).length() >= kMinAnnotationSize) {
                arrowAnnotations_.append({localStart, localEnd});
                annotationHistory_.append(AnnotationKind::Arrow);
                if (toolbar_) {
                    toolbar_->setUndoEnabled(true);
                }
            }
        } else if (!rect.isNull() && rect.width() >= kMinAnnotationSize && rect.height() >= kMinAnnotationSize) {
            const QRect localRect = rect.translated(-selectionRect_.topLeft());
            if (mosaicMode_) {
                mosaicRects_.append(localRect);
                annotationHistory_.append(AnnotationKind::Mosaic);
            } else {
                annotationRects_.append(localRect);
                annotationHistory_.append(AnnotationKind::Rectangle);
            }
            if (toolbar_) {
                toolbar_->setUndoEnabled(true);
            }
        }
        annotationRect_ = QRect();
        arrowStart_ = QPoint();
        arrowEnd_ = QPoint();
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
        if (textEditor_->isVisible()) {
            commitTextInput();
            return;
        }
        if (textMode_) {
            setTextMode(false);
            return;
        }
        if (mosaicMode_) {
            setMosaicMode(false);
            return;
        }
        if (arrowMode_) {
            setArrowMode(false);
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
    if (textMode_) {
        setCursor(selectionRect_.contains(pos) ? Qt::IBeamCursor : Qt::ArrowCursor);
        return;
    }
    if (mosaicMode_) {
        setCursor(selectionRect_.contains(pos) ? Qt::CrossCursor : Qt::ArrowCursor);
        return;
    }
    if (arrowMode_) {
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
    arrowAnnotations_.clear();
    mosaicRects_.clear();
    textAnnotations_.clear();
    annotationHistory_.clear();
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
    toolbar_->setArrowMode(arrowMode_);
    toolbar_->setTextMode(textMode_);
    toolbar_->setMosaicMode(mosaicMode_);
    toolbar_->setUndoEnabled(!annotationHistory_.isEmpty());
    toolbar_->setCurrentTextSize(currentTextSize_);
    toolbar_->setCurrentTextColor(currentTextColor_);
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
    commitTextInput();
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
    if (enabled) {
        arrowMode_ = false;
        textMode_ = false;
        mosaicMode_ = false;
        cancelTextInput();
    }
    drawingAnnotation_ = false;
    annotationRect_ = QRect();
    if (toolbar_) {
        toolbar_->setRectangleMode(enabled);
        if (enabled) {
            toolbar_->setArrowMode(false);
            toolbar_->setTextMode(false);
            toolbar_->setMosaicMode(false);
        }
    }
    setCursor(enabled ? Qt::CrossCursor : Qt::SizeAllCursor);
    update();
}

void ScreenshotOverlay::toggleArrowMode() {
    setArrowMode(!arrowMode_);
}

void ScreenshotOverlay::setArrowMode(bool enabled) {
    arrowMode_ = enabled;
    if (enabled) {
        rectangleMode_ = false;
        textMode_ = false;
        mosaicMode_ = false;
        drawingAnnotation_ = false;
        annotationRect_ = QRect();
        cancelTextInput();
    }
    if (toolbar_) {
        toolbar_->setArrowMode(enabled);
        if (enabled) {
            toolbar_->setRectangleMode(false);
            toolbar_->setTextMode(false);
            toolbar_->setMosaicMode(false);
        }
    }
    setCursor(enabled ? Qt::CrossCursor : Qt::SizeAllCursor);
    update();
}

void ScreenshotOverlay::toggleTextMode() {
    setTextMode(!textMode_);
}

void ScreenshotOverlay::setTextMode(bool enabled) {
    textMode_ = enabled;
    if (enabled) {
        rectangleMode_ = false;
        arrowMode_ = false;
        mosaicMode_ = false;
        drawingAnnotation_ = false;
        annotationRect_ = QRect();
    } else {
        cancelTextInput();
    }
    if (toolbar_) {
        toolbar_->setTextMode(enabled);
        if (enabled) {
            toolbar_->setRectangleMode(false);
            toolbar_->setArrowMode(false);
            toolbar_->setMosaicMode(false);
        }
    }
    setCursor(enabled ? Qt::IBeamCursor : Qt::SizeAllCursor);
    update();
}

void ScreenshotOverlay::toggleMosaicMode() {
    setMosaicMode(!mosaicMode_);
}

void ScreenshotOverlay::setMosaicMode(bool enabled) {
    mosaicMode_ = enabled;
    if (enabled) {
        rectangleMode_ = false;
        arrowMode_ = false;
        textMode_ = false;
        drawingAnnotation_ = false;
        annotationRect_ = QRect();
        cancelTextInput();
    }
    if (toolbar_) {
        toolbar_->setMosaicMode(enabled);
        if (enabled) {
            toolbar_->setRectangleMode(false);
            toolbar_->setArrowMode(false);
            toolbar_->setTextMode(false);
        }
    }
    setCursor(enabled ? Qt::CrossCursor : Qt::SizeAllCursor);
    update();
}

void ScreenshotOverlay::undoLastAnnotation() {
    if (annotationHistory_.isEmpty()) {
        return;
    }
    const AnnotationKind kind = annotationHistory_.takeLast();
    if (kind == AnnotationKind::Rectangle && !annotationRects_.isEmpty()) {
        annotationRects_.removeLast();
    } else if (kind == AnnotationKind::Arrow && !arrowAnnotations_.isEmpty()) {
        arrowAnnotations_.removeLast();
    } else if (kind == AnnotationKind::Mosaic && !mosaicRects_.isEmpty()) {
        mosaicRects_.removeLast();
    } else if (kind == AnnotationKind::Text && !textAnnotations_.isEmpty()) {
        textAnnotations_.removeLast();
    }
    if (toolbar_) {
        toolbar_->setUndoEnabled(!annotationHistory_.isEmpty());
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
    if (rectangleMode_ && drawingAnnotation_) {
        drawRect(normalizedAnnotationRect());
    }
}

void ScreenshotOverlay::drawArrowAnnotations(QPainter &painter, const QPoint &offset) const {
    for (const ArrowAnnotation &annotation : arrowAnnotations_) {
        drawArrowShape(painter, annotation.start, annotation.end, selectionRect_.topLeft() + offset);
    }
    if (arrowMode_ && drawingAnnotation_) {
        drawArrowShape(painter, arrowStart_, arrowEnd_);
    }
}

void ScreenshotOverlay::drawMosaicAnnotations(QPainter &painter) const {
    for (const QRect &rect : mosaicRects_) {
        drawMosaicRegion(painter, screenPixmap_, rect.translated(selectionRect_.topLeft()));
    }
    if (mosaicMode_ && drawingAnnotation_) {
        drawMosaicRegion(painter, screenPixmap_, normalizedAnnotationRect());
    }
}

void ScreenshotOverlay::drawTextAnnotations(QPainter &painter, const QPoint &offset) const {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const TextAnnotation &annotation : textAnnotations_) {
        QFont font = painter.font();
        font.setPointSize(annotation.pointSize);
        painter.setFont(font);
        painter.setPen(annotation.color);
        const QFontMetrics metrics(font);
        const QPoint drawPos = selectionRect_.topLeft() + offset + annotation.pos + QPoint(0, metrics.ascent());
        painter.drawText(drawPos, annotation.text);
    }
    painter.restore();
}

QPixmap ScreenshotOverlay::renderedSelectionPixmap() const {
    QPixmap pixmap = ScreenImage::crop(screenPixmap_, selectionRect_);
    if (pixmap.isNull()) {
        return pixmap;
    }

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const QRect &rect : mosaicRects_) {
        drawMosaicRegion(painter, pixmap, rect);
    }
    for (const QRect &rect : annotationRects_) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(239, 68, 68, 70), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
        painter.setPen(QPen(QColor(220, 38, 38, 230), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
    }
    drawArrowAnnotations(painter, -selectionRect_.topLeft());
    drawTextAnnotations(painter, -selectionRect_.topLeft());
    painter.end();
    return pixmap;
}

void ScreenshotOverlay::beginTextInput(const QPoint &pos) {
    if (!textEditor_ || !selectionRect_.contains(pos)) {
        return;
    }
    commitTextInput();

    QFont font = textEditor_->font();
    font.setPointSize(currentTextSize_);
    textEditor_->setFont(font);
    QPalette palette = textEditor_->palette();
    palette.setColor(QPalette::Text, currentTextColor_);
    textEditor_->setPalette(palette);
    const int width = std::max(120, selectionRect_.width() / 3);
    const int x = std::clamp(pos.x(), selectionRect_.left(), std::max(selectionRect_.left(), selectionRect_.right() - width));
    const int y = std::clamp(pos.y(), selectionRect_.top(), std::max(selectionRect_.top(), selectionRect_.bottom() - 36));
    textEditor_->setGeometry(x, y, width, 32);
    textEditor_->clear();
    textEditor_->show();
    textEditor_->raise();
    textEditor_->setFocus();
}

void ScreenshotOverlay::commitTextInput() {
    if (!textEditor_ || !textEditor_->isVisible()) {
        return;
    }
    const QString text = textEditor_->text().trimmed();
    if (!text.isEmpty()) {
        TextAnnotation annotation;
        annotation.text = text;
        annotation.pos = textEditor_->geometry().topLeft() - selectionRect_.topLeft();
        annotation.pointSize = currentTextSize_;
        annotation.color = currentTextColor_;
        textAnnotations_.append(annotation);
        annotationHistory_.append(AnnotationKind::Text);
        if (toolbar_) {
            toolbar_->setUndoEnabled(true);
        }
    }
    textEditor_->hide();
    textEditor_->clear();
    update();
}

void ScreenshotOverlay::cancelTextInput() {
    if (!textEditor_) {
        return;
    }
    textEditor_->hide();
    textEditor_->clear();
}
