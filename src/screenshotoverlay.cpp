#include "screenshotoverlay.h"

#include "selectiontoolbar.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

namespace {
constexpr QColor kShadeColor(0, 0, 0, 140);
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
    hide();
}

void ScreenshotOverlay::beginCapture(const QPixmap &screenPixmap, const QRect &virtualGeometry) {
    screenPixmap_ = screenPixmap;
    virtualGeometry_ = virtualGeometry;
    selectionRect_ = QRect();
    selecting_ = false;
    selectionAnchor_ = QPoint();
    selectionLocked_ = false;
    hideToolbar();
    setGeometry(virtualGeometry_);
    show();
    raise();
    activateWindow();
}

void ScreenshotOverlay::resetState() {
    selectionRect_ = QRect();
    selecting_ = false;
    selectionAnchor_ = QPoint();
    selectionLocked_ = false;
    screenPixmap_ = QPixmap();
    hideToolbar();
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
        if (event->button() == Qt::LeftButton && !selectionRect_.contains(event->pos())) {
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
    if (!selecting_) {
        return;
    }
    selectionRect_ = QRect(selectionAnchor_, event->pos());
    update();
}

void ScreenshotOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !selecting_) {
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

void ScreenshotOverlay::finalizeSelection() {
    QRect area = normalizedSelection();
    if (area.isNull() || area.width() < 5 || area.height() < 5) {
        cancelCaptureInternal();
        return;
    }

    selectionRect_ = area;
    selectionLocked_ = true;
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
