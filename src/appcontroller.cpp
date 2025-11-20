#include "appcontroller.h"

#include "screenshotoverlay.h"
#include "pinnedshot.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QScreen>
#include <algorithm>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

namespace {
QIcon buildTrayIcon() {
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(220, 40, 40));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect().adjusted(6, 6, -6, -6), 10, 10);
    painter.setPen(QPen(Qt::white, 4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPoint(14, 34), QPoint(34, 14));
    painter.drawLine(QPoint(22, 14), QPoint(34, 14));
    painter.end();
    return QIcon(pixmap);
}
}

#ifdef Q_OS_MAC
struct AppController::MacHotkeyData {
    EventHotKeyRef screenshotRef = nullptr;
    EventHotKeyRef pinRef = nullptr;
    EventHandlerRef handlerRef = nullptr;
};

static OSStatus hotkeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData) {
    Q_UNUSED(nextHandler);
    auto *controller = static_cast<AppController *>(userData);
    EventHotKeyID hotKeyID;
    GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(hotKeyID), nullptr, &hotKeyID);
    if (hotKeyID.signature == 'cutc') {
        controller->handleHotkey(static_cast<int>(hotKeyID.id));
    }
    return noErr;
}
#endif

AppController::AppController(QObject *parent)
    : QObject(parent) {
    overlay_ = new ScreenshotOverlay;
    connect(overlay_, &ScreenshotOverlay::captureConfirmed, this, &AppController::onCaptureConfirmed);
    connect(overlay_, &ScreenshotOverlay::captureCanceled, this, &AppController::onCaptureCanceled);
    connect(overlay_, &ScreenshotOverlay::pinRequested, this, &AppController::onPinRequested);
}

AppController::~AppController() {
    unregisterHotkeys();
    delete overlay_;
}

bool AppController::initialize() {
    setupTray();
    if (!registerHotkeys()) {
        tray_->showMessage(tr("CutCut"), tr("Failed to register global hotkeys."));
        return false;
    }
    return true;
}

void AppController::setupTray() {
    tray_ = new QSystemTrayIcon(buildTrayIcon(), this);

    auto *menu = new QMenu;
    QAction *captureAction = menu->addAction(tr("Capture (F1)"));
    QAction *pinAction = menu->addAction(tr("Pin (F3)"));
    menu->addSeparator();
    QAction *quitAction = menu->addAction(tr("Quit"));

    connect(captureAction, &QAction::triggered, this, &AppController::startCapture);
    connect(pinAction, &QAction::triggered, this, &AppController::pinCapture);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    tray_->setContextMenu(menu);
    tray_->setToolTip(tr("CutCut"));
    tray_->show();
}

void AppController::startCapture() {
    if (overlay_->isVisible()) {
        return;
    }
    const QRect geometry = virtualScreenGeometry();
    if (geometry.isNull()) {
        return;
    }
    const QPixmap screen = captureVirtualScreen(geometry);
    if (screen.isNull()) {
        return;
    }
    overlay_->beginCapture(screen, geometry);
}

void AppController::onCaptureConfirmed(const QPixmap &pixmap) {
    QGuiApplication::clipboard()->setPixmap(pixmap);
    lastCapture_ = pixmap;
    tray_->showMessage(tr("CutCut"), tr("Screenshot copied. Press F3 to pin."), QSystemTrayIcon::Information, 2000);
}

void AppController::onCaptureCanceled() {
    tray_->showMessage(tr("CutCut"), tr("Capture canceled."), QSystemTrayIcon::Information, 1500);
}

void AppController::onPinRequested(const QPixmap &pixmap) {
    spawnPin(pixmap);
}

bool AppController::registerHotkeys() {
#ifdef Q_OS_WIN
    if (hotkeysRegistered_) {
        return true;
    }
    if (!RegisterHotKey(nullptr, ScreenshotHotkey, 0, VK_F1)) {
        return false;
    }
    if (!RegisterHotKey(nullptr, PinHotkey, 0, VK_F3)) {
        UnregisterHotKey(nullptr, ScreenshotHotkey);
        return false;
    }
    qApp->installNativeEventFilter(this);
    hotkeysRegistered_ = true;
    return true;
#elif defined(Q_OS_MAC)
    if (macHotkeys_) {
        return true;
    }
    macHotkeys_ = new MacHotkeyData;
    EventTypeSpec eventSpec = {kEventClassKeyboard, kEventHotKeyPressed};
    InstallApplicationEventHandler(hotkeyHandler, 1, &eventSpec, this, &macHotkeys_->handlerRef);

    EventHotKeyID shotId{ 'cutc', ScreenshotHotkey };
    EventHotKeyID pinId{ 'cutc', PinHotkey };
    if (RegisterEventHotKey(kVK_F1, 0, shotId, GetApplicationEventTarget(), 0, &macHotkeys_->screenshotRef) != noErr) {
        unregisterHotkeys();
        return false;
    }
    if (RegisterEventHotKey(kVK_F3, 0, pinId, GetApplicationEventTarget(), 0, &macHotkeys_->pinRef) != noErr) {
        unregisterHotkeys();
        return false;
    }
    return true;
#else
    return false;
#endif
}

void AppController::unregisterHotkeys() {
#ifdef Q_OS_WIN
    if (hotkeysRegistered_) {
        UnregisterHotKey(nullptr, ScreenshotHotkey);
        UnregisterHotKey(nullptr, PinHotkey);
        qApp->removeNativeEventFilter(this);
        hotkeysRegistered_ = false;
    }
#elif defined(Q_OS_MAC)
    if (!macHotkeys_) {
        return;
    }
    if (macHotkeys_->screenshotRef) {
        UnregisterEventHotKey(macHotkeys_->screenshotRef);
    }
    if (macHotkeys_->pinRef) {
        UnregisterEventHotKey(macHotkeys_->pinRef);
    }
    if (macHotkeys_->handlerRef) {
        RemoveEventHandler(macHotkeys_->handlerRef);
    }
    delete macHotkeys_;
    macHotkeys_ = nullptr;
#endif
}

bool AppController::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
    Q_UNUSED(result);
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        const MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY) {
            handleHotkey(static_cast<int>(msg->wParam));
            return true;
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return false;
}

void AppController::handleHotkey(int hotkeyId) {
    if (hotkeyId == ScreenshotHotkey) {
        startCapture();
    } else if (hotkeyId == PinHotkey) {
        pinCapture();
    }
}

void AppController::pinCapture() {
    if (overlay_->hasSelection() && overlay_->copySelection(true)) {
        return;
    }
    if (lastCapture_.isNull()) {
        tray_->showMessage(tr("CutCut"), tr("Take a screenshot first (F1)."), QSystemTrayIcon::Information, 2000);
        return;
    }
    spawnPin(lastCapture_);
}

void AppController::spawnPin(const QPixmap &pixmap) {
    if (pixmap.isNull()) {
        return;
    }
    lastCapture_ = pixmap;
    auto *pin = new PinnedShot(pixmap);
    pin->show();
    pins_.append(pin);
    connect(pin, &QObject::destroyed, this, [this, pin]() { pins_.removeOne(pin); });
    if (overlay_->hasSelection()) {
        overlay_->dismiss();
    }
}

QRect AppController::virtualScreenGeometry() const {
    QRect geometry;
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        geometry = geometry.united(screen->geometry());
    }
    if (geometry.isNull()) {
        geometry = QGuiApplication::primaryScreen()->geometry();
    }
    return geometry;
}

QPixmap AppController::captureVirtualScreen(const QRect &geometry) const {
    if (geometry.isNull()) {
        return {};
    }

    const auto screens = QGuiApplication::screens();
    qreal maxDpr = 1.0;
    for (QScreen *screen : screens) {
        maxDpr = std::max(maxDpr, screen->devicePixelRatio());
    }

    QSize scaledSize = geometry.size();
    scaledSize.setWidth(int(scaledSize.width() * maxDpr));
    scaledSize.setHeight(int(scaledSize.height() * maxDpr));
    QPixmap canvas(scaledSize);
    canvas.setDevicePixelRatio(maxDpr);
    canvas.fill(Qt::transparent);

    QPainter painter(&canvas);
    painter.scale(maxDpr, maxDpr);
    for (QScreen *screen : screens) {
        const QRect screenRect = screen->geometry();
        const QPoint targetPos = screenRect.topLeft() - geometry.topLeft();
        painter.drawPixmap(targetPos, screen->grabWindow(0));
    }
    return canvas;
}
