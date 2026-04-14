#include "selectiontoolbar.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <QToolButton>
#include <algorithm>

namespace {
constexpr int kToolbarMargin = 14;
constexpr int kButtonSize = 34;
constexpr int kIconSize = 22;

class GripHandle final : public QWidget {
public:
    explicit GripHandle(QWidget *parent = nullptr)
        : QWidget(parent) {
        setFixedSize(24, 18);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        static const QIcon gripIcon = [] {
            constexpr auto kGripSvg = R"svg(
<svg width="18" height="18" viewBox="0 0 18 18" fill="none" xmlns="http://www.w3.org/2000/svg">
  <circle cx="4.5" cy="6.3" r="1.05" fill="#97A0B2"/>
  <circle cx="9" cy="6.3" r="1.05" fill="#97A0B2"/>
  <circle cx="13.5" cy="6.3" r="1.05" fill="#97A0B2"/>
  <circle cx="4.5" cy="11.3" r="1.05" fill="#97A0B2"/>
  <circle cx="9" cy="11.3" r="1.05" fill="#97A0B2"/>
  <circle cx="13.5" cy="11.3" r="1.05" fill="#97A0B2"/>
</svg>
)svg";
            QPixmap pixmap(18, 18);
            pixmap.fill(Qt::transparent);
            QSvgRenderer renderer{QByteArray(kGripSvg)};
            QPainter painter(&pixmap);
            renderer.render(&painter);
            return QIcon(pixmap);
        }();

        QPainter painter(this);
        gripIcon.paint(&painter, rect(), Qt::AlignCenter);
    }
};

QIcon makeSvgIcon(const char *svg, const QSize &size) {
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QSvgRenderer renderer{QByteArray(svg)};
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));

    return QIcon(pixmap);
}

QIcon makePinIcon() {
    constexpr auto kPinSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M11 15.6V20.2" stroke="#374151" stroke-width="1.85" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M8.3 10.65A1.85 1.85 0 0 1 7.27 12.31L5.62 13.14A1.85 1.85 0 0 0 4.6 14.79V15.5A0.92 0.92 0 0 0 5.52 16.42H16.48A0.92 0.92 0 0 0 17.4 15.5V14.79A1.85 1.85 0 0 0 16.38 13.14L14.73 12.31A1.85 1.85 0 0 1 13.7 10.65V7.15A0.92 0.92 0 0 1 14.62 6.23A1.85 1.85 0 0 0 14.62 2.53H7.38A1.85 1.85 0 0 0 7.38 6.23A0.92 0.92 0 0 1 8.3 7.15V10.65Z" stroke="#374151" stroke-width="1.85" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kPinSvg, QSize(kIconSize, kIconSize));
}

QIcon makeRectangleIcon() {
    constexpr auto kRectangleSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <rect x="2.7" y="5.7" width="16.6" height="10.6" rx="1.8" stroke="#374151" stroke-width="1.85" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kRectangleSvg, QSize(kIconSize, kIconSize));
}

QIcon makeCloseIcon() {
    constexpr auto kCloseSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M16.5 5.5L5.5 16.5" stroke="#E05555" stroke-width="1.95" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M5.5 5.5L16.5 16.5" stroke="#E05555" stroke-width="1.95" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kCloseSvg, QSize(kIconSize, kIconSize));
}

QIcon makeCheckIcon() {
    constexpr auto kCheckSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M18.3 5.7L9.3 14.7L5.2 10.6" stroke="#2EAA64" stroke-width="1.95" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kCheckSvg, QSize(kIconSize, kIconSize));
}

QToolButton *makeIconButton(const QIcon &icon, const QString &tooltip) {
    auto *button = new QToolButton;
    button->setIcon(icon);
    button->setIconSize(QSize(kIconSize, kIconSize));
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoRaise(true);
    button->setFixedSize(kButtonSize, kButtonSize);
    button->setStyleSheet("QToolButton {"
                          "background: transparent;"
                          "border: none;"
                          "border-radius: 8px;"
                          "}"
                          "QToolButton:hover {"
                          "background: rgba(27, 31, 38, 0.08);"
                          "}"
                          "QToolButton:pressed {"
                          "background: rgba(27, 31, 38, 0.14);"
                          "}");
    return button;
}

QFrame *makeDivider() {
    auto *divider = new QFrame;
    divider->setFrameShape(QFrame::VLine);
    divider->setFixedHeight(16);
    divider->setStyleSheet("color: rgba(27, 31, 38, 0.10);");
    return divider;
}
}

SelectionToolbar::SelectionToolbar(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    setVisible(false);
    setCursor(Qt::OpenHandCursor);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(12, 18, 28, 42));
    setGraphicsEffect(shadow);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 4, 12, 4);
    layout->setSpacing(1);

    auto *grip = new GripHandle(this);

    rectangleButton_ = makeIconButton(makeRectangleIcon(), tr("Rectangle"));
    pinButton_ = makeIconButton(makePinIcon(), tr("Pin"));
    copyButton_ = makeIconButton(makeCheckIcon(), tr("Copy"));
    closeButton_ = makeIconButton(makeCloseIcon(), tr("Cancel"));
    rectangleButton_->setCheckable(true);

    layout->addWidget(grip);
    layout->addSpacing(3);
    layout->addWidget(makeDivider());
    layout->addSpacing(3);
    layout->addWidget(rectangleButton_);
    layout->addWidget(pinButton_);
    layout->addWidget(closeButton_);
    layout->addWidget(copyButton_);

    connect(rectangleButton_, &QToolButton::clicked, this, &SelectionToolbar::rectangleClicked);
    connect(copyButton_, &QToolButton::clicked, this, &SelectionToolbar::copyClicked);
    connect(pinButton_, &QToolButton::clicked, this, &SelectionToolbar::pinClicked);
    connect(closeButton_, &QToolButton::clicked, this, &SelectionToolbar::closeClicked);
}

void SelectionToolbar::setRectangleMode(bool enabled) {
    if (!rectangleButton_) {
        return;
    }
    rectangleButton_->setChecked(enabled);
    rectangleButton_->setStyleSheet(enabled
                                        ? "QToolButton {"
                                          "background: rgba(220, 38, 38, 0.12);"
                                          "border: none;"
                                          "border-radius: 8px;"
                                          "}"
                                          "QToolButton:hover {"
                                          "background: rgba(220, 38, 38, 0.16);"
                                          "}"
                                          "QToolButton:pressed {"
                                          "background: rgba(220, 38, 38, 0.22);"
                                          "}"
                                        : "QToolButton {"
                                          "background: transparent;"
                                          "border: none;"
                                          "border-radius: 8px;"
                                          "}"
                                          "QToolButton:hover {"
                                          "background: rgba(27, 31, 38, 0.08);"
                                          "}"
                                          "QToolButton:pressed {"
                                          "background: rgba(27, 31, 38, 0.14);"
                                          "}");
}

void SelectionToolbar::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(15, 23, 42, 23), 1));
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
}

void SelectionToolbar::showForSelection(const QRect &selection, const QRect &overlayRect) {
    overlayRect_ = overlayRect;
    adjustSize();

    const int minY = overlayRect.top() + kToolbarMargin;
    const int yMax = std::max(minY, overlayRect.bottom() - height() - kToolbarMargin);
    int y = selection.bottom() + kToolbarMargin;
    if (y > yMax) {
        y = selection.top() - height() - kToolbarMargin;
    }
    y = std::clamp(y, minY, yMax);

    const int minX = overlayRect.left() + kToolbarMargin;
    const int maxX = std::max(minX, overlayRect.right() - width() - kToolbarMargin);
    int x = selection.center().x() - width() / 2;
    x = std::clamp(x, minX, maxX);

    move(x, y);
    show();
    raise();
}

void SelectionToolbar::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    dragging_ = true;
    dragOffset_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void SelectionToolbar::mouseMoveEvent(QMouseEvent *event) {
    if (!dragging_) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    move(clampPosition(pos() + event->pos() - dragOffset_));
    event->accept();
}

void SelectionToolbar::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

QPoint SelectionToolbar::clampPosition(const QPoint &pos) const {
    if (overlayRect_.isNull()) {
        return pos;
    }

    const int minX = overlayRect_.left() + kToolbarMargin;
    const int maxX = std::max(minX, overlayRect_.right() - width() - kToolbarMargin);
    const int minY = overlayRect_.top() + kToolbarMargin;
    const int maxY = std::max(minY, overlayRect_.bottom() - height() - kToolbarMargin);

    return QPoint(std::clamp(pos.x(), minX, maxX),
                  std::clamp(pos.y(), minY, maxY));
}
