#pragma once

#include <QPixmap>
#include <QWidget>

class SelectionToolbar;

class ScreenshotOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ScreenshotOverlay(QWidget *parent = nullptr);

    void beginCapture(const QPixmap &screenPixmap, const QRect &virtualGeometry);
    void dismiss();
    bool hasSelection() const;
    QPixmap currentSelectionPixmap() const;
    bool copySelection(bool pin);

signals:
    void captureConfirmed(const QPixmap &pixmap);
    void captureCanceled();
    void pinRequested(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum class DragMode {
        None,
        Move,
        ResizeTop,
        ResizeBottom,
        ResizeLeft,
        ResizeRight,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight,
    };

    void resetState();
    QRect normalizedSelection() const;
    void finalizeSelection();
    void cancelCaptureInternal();
    void showToolbar();
    void hideToolbar();
    QRect clampedSelection(const QPoint &topLeft) const;
    QRect resizedSelection(const QPoint &pos) const;
    DragMode dragModeForPosition(const QPoint &pos) const;
    Qt::CursorShape cursorForDragMode(DragMode mode) const;
    void updateCursor(const QPoint &pos);

    QPixmap screenPixmap_;
    QRect virtualGeometry_;
    QRect selectionRect_;
    QPoint selectionAnchor_;
    QPoint dragOffset_;
    QPoint resizeAnchor_;
    QRect dragStartRect_;
    bool selecting_ = false;
    bool draggingSelection_ = false;
    bool selectionLocked_ = false;
    DragMode dragMode_ = DragMode::None;
    SelectionToolbar *toolbar_ = nullptr;
};
