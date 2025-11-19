#include "selectiontoolbar.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <algorithm>

SelectionToolbar::SelectionToolbar(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    setVisible(false);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(10);

    auto setupButton = [](QToolButton *button, const QString &text) {
        button->setText(text);
        button->setCursor(Qt::PointingHandCursor);
        button->setAutoRaise(false);
        button->setMinimumWidth(70);
        button->setMinimumHeight(28);
        button->setStyleSheet("QToolButton {"
                              "color: white;"
                              "background: rgba(255, 255, 255, 0.1);"
                              "border: 1px solid rgba(255,255,255,0.35);"
                              "border-radius: 6px;"
                              "font-weight: 500;"
                              "}"
                              "QToolButton:hover {"
                              "background: rgba(255,255,255,0.25);"
                              "border-color: rgba(255,255,255,0.6);"
                              "}"
                              "QToolButton:pressed {"
                              "background: rgba(255,255,255,0.4);"
                              "}");
    };

    pinButton_ = new QToolButton(this);
    setupButton(pinButton_, tr("Pin"));

    copyButton_ = new QToolButton(this);
    setupButton(copyButton_, tr("Copy"));

    layout->addWidget(pinButton_);
    layout->addWidget(copyButton_);

    setStyleSheet("SelectionToolbar { background: rgba(20, 20, 20, 180); border-radius: 8px; }");

    connect(copyButton_, &QToolButton::clicked, this, &SelectionToolbar::copyClicked);
    connect(pinButton_, &QToolButton::clicked, this, &SelectionToolbar::pinClicked);
}

void SelectionToolbar::showForSelection(const QRect &selection, const QRect &overlayRect) {
    adjustSize();
    const int margin = 10;
    int y = selection.bottom() + margin;
    if (y + height() > overlayRect.bottom() - margin) {
        y = selection.top() - height() - margin;
    }
    int x = selection.center().x() - width() / 2;
    x = std::clamp(x, overlayRect.left() + margin, overlayRect.right() - width() - margin);
    move(x, y);
    show();
    raise();
}
