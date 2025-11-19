#pragma once

#include <QWidget>

class QToolButton;

class SelectionToolbar : public QWidget {
    Q_OBJECT
public:
    explicit SelectionToolbar(QWidget *parent = nullptr);

    void showForSelection(const QRect &selection, const QRect &overlayRect);

signals:
    void copyClicked();
    void pinClicked();

private:
    QToolButton *copyButton_ = nullptr;
    QToolButton *pinButton_ = nullptr;
};
