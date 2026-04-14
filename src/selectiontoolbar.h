#pragma once

#include <QWidget>

class QMouseEvent;
class QToolButton;

class SelectionToolbar : public QWidget {
    Q_OBJECT
public:
    explicit SelectionToolbar(QWidget *parent = nullptr);

    void showForSelection(const QRect &selection, const QRect &overlayRect);
    void setRectangleMode(bool enabled);

signals:
    void rectangleClicked();
    void copyClicked();
    void pinClicked();
    void closeClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint clampPosition(const QPoint &pos) const;

    QRect overlayRect_;
    bool dragging_ = false;
    QPoint dragOffset_;
    QToolButton *rectangleButton_ = nullptr;
    QToolButton *copyButton_ = nullptr;
    QToolButton *pinButton_ = nullptr;
    QToolButton *closeButton_ = nullptr;
};
