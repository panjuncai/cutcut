#pragma once

#include <QColor>
#include <QPixmap>
#include <QVector>
#include <QWidget>

class QLineEdit;
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
    void toggleRectangleMode();
    void setRectangleMode(bool enabled);
    void toggleTextMode();
    void setTextMode(bool enabled);
    void toggleMosaicMode();
    void setMosaicMode(bool enabled);
    void undoLastAnnotation();
    QRect normalizedAnnotationRect() const;
    void drawAnnotationRectangles(QPainter &painter) const;
    void drawMosaicAnnotations(QPainter &painter) const;
    void drawTextAnnotations(QPainter &painter, const QPoint &offset = QPoint()) const;
    QPixmap renderedSelectionPixmap() const;
    void beginTextInput(const QPoint &pos);
    void commitTextInput();
    void cancelTextInput();

    struct TextAnnotation {
        QString text;
        QPoint pos;
        int pointSize = 18;
        QColor color = QColor("#DC2626");
    };

    enum class AnnotationKind {
        Rectangle,
        Text,
        Mosaic,
    };

    QPixmap screenPixmap_;
    QRect virtualGeometry_;
    QRect selectionRect_;
    QRect annotationRect_;
    QPoint selectionAnchor_;
    QPoint dragOffset_;
    QPoint resizeAnchor_;
    QPoint annotationAnchor_;
    QRect dragStartRect_;
    QVector<QRect> annotationRects_;
    QVector<QRect> mosaicRects_;
    QVector<TextAnnotation> textAnnotations_;
    QVector<AnnotationKind> annotationHistory_;
    QLineEdit *textEditor_ = nullptr;
    int currentTextSize_ = 18;
    QColor currentTextColor_ = QColor("#DC2626");
    bool selecting_ = false;
    bool drawingAnnotation_ = false;
    bool rectangleMode_ = false;
    bool textMode_ = false;
    bool mosaicMode_ = false;
    bool draggingSelection_ = false;
    bool selectionLocked_ = false;
    DragMode dragMode_ = DragMode::None;
    SelectionToolbar *toolbar_ = nullptr;
};
