#include "selectiontoolbar.h"

#include <QComboBox>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr int kToolbarMargin = 14;
constexpr int kButtonSize = 34;
constexpr int kIconSize = 22;
constexpr int kSettingsSwatchSize = 24;

QIcon makeSvgIcon(const char *svg, const QSize &size) {
    QSvgRenderer renderer{QByteArray(svg)};
    QIcon icon;
    // Render each density directly from vectors; never enlarge the 1x bitmap.
    for (qreal ratio : {1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 4.0}) {
        QPixmap pixmap(qRound(size.width() * ratio), qRound(size.height() * ratio));
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter, QRectF(QPointF(), QSizeF(size)));
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

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
            return makeSvgIcon(kGripSvg, QSize(18, 18));
        }();

        QPainter painter(this);
        gripIcon.paint(&painter, rect(), Qt::AlignCenter);
    }
};

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

QIcon makeArrowIcon() {
    constexpr auto kArrowSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M10.2 17.4H4.8V12" stroke="#374151" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M17.2 4.8L4.8 17.2" stroke="#374151" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kArrowSvg, QSize(kIconSize, kIconSize));
}

QIcon makeUndoIcon() {
    constexpr auto kUndoSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M8.5 12.8L4 8.3L8.5 3.8" stroke="#374151" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M4 8.3H13.7A4.95 4.95 0 0 1 18.65 13.25A4.95 4.95 0 0 1 13.7 18.2H10.6" stroke="#374151" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kUndoSvg, QSize(kIconSize, kIconSize));
}

QIcon makeTypeIcon() {
    constexpr auto kTypeSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M11 3.8V18.2" stroke="#374151" stroke-width="1.95" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M4.5 6.5V4.8C4.5 4.19 4.99 3.7 5.6 3.7H16.4C17.01 3.7 17.5 4.19 17.5 4.8V6.5" stroke="#374151" stroke-width="1.95" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M8.4 18.2H13.6" stroke="#374151" stroke-width="1.95" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)svg";
    return makeSvgIcon(kTypeSvg, QSize(kIconSize, kIconSize));
}

QIcon makeMosaicIcon() {
    constexpr auto kMosaicSvg = R"svg(
<svg width="22" height="22" viewBox="0 0 22 22" fill="none" xmlns="http://www.w3.org/2000/svg">
  <rect x="3" y="3" width="4" height="4" rx="0.5" fill="#374151"/>
  <rect x="9" y="3" width="4" height="4" rx="0.5" fill="#9CA3AF"/>
  <rect x="16" y="2" width="4" height="4" rx="0.5" fill="#6B7280"/>
  <rect x="3" y="9" width="4" height="4" rx="0.5" fill="#9CA3AF"/>
  <rect x="9" y="9" width="4" height="4" rx="0.5" fill="#4B5563"/>
  <rect x="15" y="9" width="4" height="4" rx="0.5" fill="#D1D5DB"/>
  <rect x="3" y="15" width="4" height="4" rx="0.5" fill="#4B5563"/>
  <rect x="9" y="15" width="4" height="4" rx="0.5" fill="#D1D5DB"/>
  <rect x="15" y="15" width="4" height="4" rx="0.5" fill="#6B7280"/>
</svg>
)svg";
    return makeSvgIcon(kMosaicSvg, QSize(kIconSize, kIconSize));
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

QToolButton *makeColorButton(const QColor &color) {
    auto *button = new QToolButton;
    button->setCheckable(true);
    button->setAutoRaise(true);
    button->setFixedSize(kSettingsSwatchSize, kSettingsSwatchSize);
    button->setCursor(Qt::PointingHandCursor);
    const QString border = color == Qt::white ? "rgba(107, 114, 128, 0.35)" : "transparent";
    button->setStyleSheet(QString("QToolButton {"
                                  "background: %1;"
                                  "border: 1px solid %2;"
                                  "border-radius: 6px;"
                                  "}"
                                  "QToolButton:checked {"
                                  "border: 2px solid rgba(239, 68, 68, 0.95);"
                                  "}"
                                  "QToolButton:hover {"
                                  "border: 2px solid rgba(148, 163, 184, 0.8);"
                                  "}").arg(color.name(), border));
    return button;
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

    rectangleButton_ = makeIconButton(makeRectangleIcon(), tr("矩形"));
    arrowButton_ = makeIconButton(makeArrowIcon(), tr("箭头"));
    textButton_ = makeIconButton(makeTypeIcon(), tr("文字"));
    mosaicButton_ = makeIconButton(makeMosaicIcon(), tr("马赛克"));
    undoButton_ = makeIconButton(makeUndoIcon(), tr("撤销"));
    pinButton_ = makeIconButton(makePinIcon(), tr("钉图"));
    copyButton_ = makeIconButton(makeCheckIcon(), tr("复制"));
    closeButton_ = makeIconButton(makeCloseIcon(), tr("取消"));
    rectangleButton_->setCheckable(true);
    arrowButton_->setCheckable(true);
    textButton_->setCheckable(true);
    mosaicButton_->setCheckable(true);
    undoButton_->setEnabled(false);

    layout->addWidget(grip);
    layout->addSpacing(3);
    layout->addWidget(makeDivider());
    layout->addSpacing(3);
    layout->addWidget(rectangleButton_);
    layout->addWidget(arrowButton_);
    layout->addWidget(textButton_);
    layout->addWidget(mosaicButton_);
    layout->addWidget(undoButton_);
    layout->addWidget(pinButton_);
    layout->addWidget(closeButton_);
    layout->addWidget(copyButton_);

    settingsPanel_ = new QWidget(parentWidget());
    settingsPanel_->hide();
    settingsPanel_->setStyleSheet("background: rgb(255, 255, 255); border-radius: 10px;");
    auto *settingsLayout = new QHBoxLayout(settingsPanel_);
    settingsLayout->setContentsMargins(10, 8, 10, 8);
    settingsLayout->setSpacing(8);

    fontSizeCombo_ = new QComboBox(settingsPanel_);
    fontSizeCombo_->addItems({"12pt", "14pt", "18pt", "24pt", "32pt"});
    fontSizeCombo_->setCurrentText("18pt");
    fontSizeCombo_->setFixedHeight(28);
    fontSizeCombo_->setStyleSheet("QComboBox {"
                                  "background: rgb(249, 250, 251);"
                                  "color: rgb(31, 41, 55);"
                                  "border: 1px solid rgba(148, 163, 184, 0.35);"
                                  "border-radius: 8px;"
                                  "padding: 2px 10px;"
                                  "min-width: 72px;"
                                  "}"
                                  "QComboBox:hover {"
                                  "color: rgb(31, 41, 55);"
                                  "}"
                                  "QComboBox::drop-down { border: none; width: 18px; }"
                                  "QComboBox QAbstractItemView {"
                                  "background: rgb(255, 255, 255);"
                                  "color: rgb(31, 41, 55);"
                                  "selection-background-color: rgba(59, 130, 246, 0.12);"
                                  "selection-color: rgb(31, 41, 55);"
                                  "border: 1px solid rgba(148, 163, 184, 0.25);"
                                  "outline: none;"
                                  "}");
    settingsLayout->addWidget(fontSizeCombo_);
    auto *settingsDivider = makeDivider();
    settingsDivider->setFixedHeight(24);
    settingsLayout->addWidget(settingsDivider);

    const QVector<QColor> colors = {
        QColor("#DC2626"),
        QColor("#D4A017"),
        QColor("#36B33E"),
        QColor("#3457EB"),
        QColor("#000000"),
        QColor("#6B7280"),
        QColor("#FFFFFF"),
    };
    for (const QColor &color : colors) {
        auto *button = makeColorButton(color);
        button->setProperty("swatchColor", color);
        colorButtons_.append(button);
        settingsLayout->addWidget(button);
    }
    if (!colorButtons_.isEmpty()) {
        colorButtons_.first()->setChecked(true);
    }

    connect(rectangleButton_, &QToolButton::clicked, this, &SelectionToolbar::rectangleClicked);
    connect(arrowButton_, &QToolButton::clicked, this, &SelectionToolbar::arrowClicked);
    connect(textButton_, &QToolButton::clicked, this, &SelectionToolbar::textClicked);
    connect(mosaicButton_, &QToolButton::clicked, this, &SelectionToolbar::mosaicClicked);
    connect(undoButton_, &QToolButton::clicked, this, &SelectionToolbar::undoClicked);
    connect(copyButton_, &QToolButton::clicked, this, &SelectionToolbar::copyClicked);
    connect(pinButton_, &QToolButton::clicked, this, &SelectionToolbar::pinClicked);
    connect(closeButton_, &QToolButton::clicked, this, &SelectionToolbar::closeClicked);
    connect(fontSizeCombo_, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        emit textSizeChanged(text.left(text.indexOf("pt")).toInt());
    });
    for (QToolButton *button : colorButtons_) {
        connect(button, &QToolButton::clicked, this, [this, button]() {
            for (QToolButton *other : colorButtons_) {
                other->setChecked(other == button);
            }
            emit textColorChanged(button->property("swatchColor").value<QColor>());
        });
    }
}

SelectionToolbar::~SelectionToolbar() {
    if (settingsPanel_) {
        settingsPanel_->deleteLater();
    }
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
    if (enabled) {
        setArrowMode(false);
        setTextMode(false);
    }
}

void SelectionToolbar::setArrowMode(bool enabled) {
    if (!arrowButton_) {
        return;
    }
    arrowButton_->setChecked(enabled);
    arrowButton_->setStyleSheet(enabled
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
    if (enabled) {
        setRectangleMode(false);
        setTextMode(false);
        setMosaicMode(false);
    }
}

void SelectionToolbar::setTextMode(bool enabled) {
    if (!textButton_ || !settingsPanel_) {
        return;
    }
    textButton_->setChecked(enabled);
    textButton_->setStyleSheet(enabled
                                   ? "QToolButton {"
                                     "background: rgba(59, 130, 246, 0.12);"
                                     "border: none;"
                                     "border-radius: 8px;"
                                     "}"
                                     "QToolButton:hover {"
                                     "background: rgba(59, 130, 246, 0.16);"
                                     "}"
                                     "QToolButton:pressed {"
                                     "background: rgba(59, 130, 246, 0.22);"
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
    settingsPanel_->setVisible(enabled);
    updateSettingsPanelPosition();
    if (enabled) {
        setRectangleMode(false);
        setArrowMode(false);
        setMosaicMode(false);
    }
}

void SelectionToolbar::setMosaicMode(bool enabled) {
    if (!mosaicButton_) {
        return;
    }
    mosaicButton_->setChecked(enabled);
    mosaicButton_->setStyleSheet(enabled
                                     ? "QToolButton {"
                                       "background: rgba(55, 65, 81, 0.12);"
                                       "border: none;"
                                       "border-radius: 8px;"
                                       "}"
                                       "QToolButton:hover {"
                                       "background: rgba(55, 65, 81, 0.16);"
                                       "}"
                                       "QToolButton:pressed {"
                                       "background: rgba(55, 65, 81, 0.22);"
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
    if (enabled) {
        setRectangleMode(false);
        setArrowMode(false);
        setTextMode(false);
    }
}

void SelectionToolbar::setUndoEnabled(bool enabled) {
    if (!undoButton_) {
        return;
    }
    undoButton_->setEnabled(enabled);
    undoButton_->setStyleSheet("QToolButton {"
                               "background: transparent;"
                               "border: none;"
                               "border-radius: 8px;"
                               "}"
                               "QToolButton:hover:enabled {"
                               "background: rgba(27, 31, 38, 0.08);"
                               "}"
                               "QToolButton:pressed:enabled {"
                               "background: rgba(27, 31, 38, 0.14);"
                               "}"
                               "QToolButton:disabled {"
                               "color: rgba(55, 65, 81, 0.38);"
                               "}");
}

void SelectionToolbar::setCurrentTextSize(int size) {
    if (!fontSizeCombo_) {
        return;
    }
    const QString label = QString::number(size) + "pt";
    const int index = fontSizeCombo_->findText(label);
    if (index >= 0) {
        fontSizeCombo_->setCurrentIndex(index);
    }
}

void SelectionToolbar::setCurrentTextColor(const QColor &color) {
    for (QToolButton *button : colorButtons_) {
        button->setChecked(button->property("swatchColor").value<QColor>() == color);
    }
}

void SelectionToolbar::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(15, 23, 42, 23), 1));
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
}

void SelectionToolbar::hideEvent(QHideEvent *event) {
    if (settingsPanel_) {
        settingsPanel_->hide();
    }
    QWidget::hideEvent(event);
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
    updateSettingsPanelPosition();
    show();
    raise();
    if (settingsPanel_ && settingsPanel_->isVisible()) {
        settingsPanel_->show();
        settingsPanel_->raise();
    }
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
    updateSettingsPanelPosition();
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

void SelectionToolbar::updateSettingsPanelPosition() {
    if (!settingsPanel_) {
        return;
    }
    settingsPanel_->adjustSize();
    if (!settingsPanel_->isVisible()) {
        return;
    }

    const int gap = 8;
    const int minX = overlayRect_.left() + kToolbarMargin;
    const int maxX = std::max(minX, overlayRect_.right() - settingsPanel_->width() - kToolbarMargin);
    const int minY = overlayRect_.top() + kToolbarMargin;
    const int maxY = std::max(minY, overlayRect_.bottom() - settingsPanel_->height() - kToolbarMargin);

    int panelX = pos().x();
    int panelY = y() + height() + gap;
    panelX = std::clamp(panelX, minX, maxX);
    panelY = std::clamp(panelY, minY, maxY);
    settingsPanel_->move(panelX, panelY);
}
