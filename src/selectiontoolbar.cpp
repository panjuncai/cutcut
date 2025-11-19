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
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(6);

    copyButton_ = new QToolButton(this);
    copyButton_->setText(tr("Copy"));
    copyButton_->setCursor(Qt::PointingHandCursor);
    copyButton_->setAutoRaise(true);

    layout->addWidget(copyButton_);

    setStyleSheet("SelectionToolbar { background: rgba(30, 30, 30, 200); border-radius: 6px; }"
                  "QToolButton { color: white; padding: 4px 10px; }"
                  "QToolButton:hover { background: rgba(255,255,255,0.15); border-radius: 4px; }");

    connect(copyButton_, &QToolButton::clicked, this, &SelectionToolbar::copyClicked);
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
