#pragma once

#include <QPixmap>
#include <QWidget>

class PinnedShot : public QWidget {
    Q_OBJECT
public:
    explicit PinnedShot(const QPixmap &pixmap, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QPixmap pixmap_;
    bool dragging_ = false;
    QPoint dragOffset_;
};
