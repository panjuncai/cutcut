#pragma once

#include <QAbstractNativeEventFilter>
#include <QPixmap>
#include <QSystemTrayIcon>

class ScreenshotOverlay;

class AppController : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    bool initialize();

public slots:
    void handleHotkey(int hotkeyId);

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void onCaptureConfirmed(const QPixmap &pixmap);
    void onCaptureCanceled();

private:
    enum HotkeyId { ScreenshotHotkey = 1 };

    void setupTray();
    void startCapture();
    bool registerHotkeys();
    void unregisterHotkeys();
    QRect virtualScreenGeometry() const;
    QPixmap captureVirtualScreen(const QRect &geometry) const;

    QSystemTrayIcon *tray_ = nullptr;
    ScreenshotOverlay *overlay_ = nullptr;

#ifdef Q_OS_WIN
    bool hotkeysRegistered_ = false;
#endif
#ifdef Q_OS_MAC
    struct MacHotkeyData;
    MacHotkeyData *macHotkeys_ = nullptr;
#endif
};
