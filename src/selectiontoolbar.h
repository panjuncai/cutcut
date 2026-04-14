#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

class QComboBox;
class QHideEvent;
class QMouseEvent;
class QToolButton;
class QWidget;

class SelectionToolbar : public QWidget {
    Q_OBJECT
public:
    explicit SelectionToolbar(QWidget *parent = nullptr);
    ~SelectionToolbar() override;

    void showForSelection(const QRect &selection, const QRect &overlayRect);
    void setRectangleMode(bool enabled);
    void setTextMode(bool enabled);
    void setUndoEnabled(bool enabled);
    void setCurrentTextSize(int size);
    void setCurrentTextColor(const QColor &color);

signals:
    void rectangleClicked();
    void textClicked();
    void undoClicked();
    void copyClicked();
    void pinClicked();
    void closeClicked();
    void textSizeChanged(int size);
    void textColorChanged(const QColor &color);

protected:
    void hideEvent(QHideEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint clampPosition(const QPoint &pos) const;
    void updateSettingsPanelPosition();

    QRect overlayRect_;
    bool dragging_ = false;
    QPoint dragOffset_;
    QWidget *settingsPanel_ = nullptr;
    QComboBox *fontSizeCombo_ = nullptr;
    QVector<QToolButton *> colorButtons_;
    QToolButton *rectangleButton_ = nullptr;
    QToolButton *textButton_ = nullptr;
    QToolButton *undoButton_ = nullptr;
    QToolButton *copyButton_ = nullptr;
    QToolButton *pinButton_ = nullptr;
    QToolButton *closeButton_ = nullptr;
};
