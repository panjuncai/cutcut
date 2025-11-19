#pragma once

#include <QPixmap>
#include <QWidget>

class ScreenshotOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ScreenshotOverlay(QWidget *parent = nullptr);

    void beginCapture(const QPixmap &screenPixmap, const QRect &virtualGeometry);
    void dismiss();
    bool hasSelection() const;
    QPixmap currentSelectionPixmap() const;

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
    void resetState();
    QRect normalizedSelection() const;
    void finalizeSelection();
    void cancelCaptureInternal();

    QPixmap screenPixmap_;
    QRect virtualGeometry_;
    QRect selectionRect_;
    QPoint selectionAnchor_;
    bool selecting_ = false;
    bool selectionLocked_ = false;
};
