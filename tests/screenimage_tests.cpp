#include "screenimage.h"
#include "screenshotoverlay.h"
#include "selectiontoolbar.h"
#include "pinnedshot.h"

#include <QMouseEvent>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QtTest>

namespace {
QImage pixels(const QPixmap &pixmap) {
    return pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
}

QPixmap patternedImage(const QSize &logicalSize, qreal ratio) {
    QImage image(qRound(logicalSize.width() * ratio), qRound(logicalSize.height() * ratio),
                 QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixel(x, y, qRgb(x % 256, y % 256, (x + y) % 256));
        }
    }
    image.setDevicePixelRatio(ratio);
    return QPixmap::fromImage(image);
}

void drag(QWidget &widget, const QPoint &from, const QPoint &to) {
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(from), QPointF(from), QPointF(from),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &press);
    QMouseEvent move(QEvent::MouseMove, QPointF(to), QPointF(to), QPointF(to),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(to), QPointF(to), QPointF(to),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &release);
}
}

class ScreenImageTests : public QObject {
    Q_OBJECT
private slots:
    void laptop150Percent() {
        const QPixmap native = patternedImage(QSize(1280, 800), 1.5);
        const QPixmap composed = ScreenImage::compose(QRect(0, 0, 1280, 800),
                                                      {{QRect(0, 0, 1280, 800), native}});
        QCOMPARE(composed.size(), QSize(1920, 1200));
        QCOMPARE(composed.devicePixelRatio(), 1.5);
        QCOMPARE(pixels(composed), pixels(native));
    }

    void selectionAndPin_data() {
        QTest::addColumn<qreal>("ratio");
        QTest::newRow("100 percent") << qreal(1);
        QTest::newRow("125 percent") << qreal(1.25);
        QTest::newRow("150 percent") << qreal(1.5);
        QTest::newRow("200 percent") << qreal(2);
    }

    void selectionAndPin() {
        QFETCH(qreal, ratio);
        const QSize logicalSize(640, 480);
        const QPixmap native = patternedImage(logicalSize, ratio);
        const QPixmap canvas = ScreenImage::compose(QRect(QPoint(), logicalSize),
                                                     {{QRect(QPoint(), logicalSize), native}});
        ScreenshotOverlay overlay;
        const auto dismiss = qScopeGuard([&overlay] { overlay.dismiss(); });
        overlay.beginCapture(canvas, QRect(QPoint(), logicalSize));
        // Odd coordinates exercise independently rounded edges at fractional DPI.
        const QRect selection(11, 13, 120, 80);
        drag(overlay, selection.topLeft(), selection.bottomRight());
        QVERIFY(overlay.hasSelection());
        const QPixmap selected = overlay.currentSelectionPixmap();
        const QRect pixelRect(qRound(11 * ratio), qRound(13 * ratio),
                              qRound(131 * ratio) - qRound(11 * ratio),
                              qRound(93 * ratio) - qRound(13 * ratio));
        QCOMPARE(selected.size(), pixelRect.size());
        QCOMPARE(selected.devicePixelRatio(), ratio);
        QCOMPARE(pixels(selected), pixels(native.copy(pixelRect)));

        PinnedShot pin(selected);
        QCOMPARE(pin.size(), (QSizeF(selected.size()) / ratio).toSize() + QSize(24, 24));

        // Applying mosaic must not resize or alter pixels outside the marked region.
        auto *toolbar = overlay.findChild<SelectionToolbar *>();
        QVERIFY(toolbar);
        toolbar->mosaicClicked();
        drag(overlay, QPoint(41, 43), QPoint(80, 72));
        const QPixmap mosaic = overlay.currentSelectionPixmap();
        QCOMPARE(mosaic.size(), selected.size());
        QCOMPARE(mosaic.devicePixelRatio(), ratio);
        QCOMPARE(mosaic.toImage().pixel(0, 0), selected.toImage().pixel(0, 0));
        QVERIFY(mosaic.toImage() != selected.toImage());

        QSignalSpy copied(&overlay, &ScreenshotOverlay::captureConfirmed);
        QSignalSpy pinned(&overlay, &ScreenshotOverlay::pinRequested);
        QVERIFY(overlay.copySelection(true));
        QCOMPARE(copied.size(), 1);
        QCOMPARE(pinned.size(), 1);
        QCOMPARE(qvariant_cast<QPixmap>(copied.first().first()).toImage(), mosaic.toImage());
        QCOMPARE(qvariant_cast<QPixmap>(pinned.first().first()).toImage(), mosaic.toImage());
    }

    void mixedScreensWithNegativeOrigin() {
        const QPixmap highDpi = patternedImage(QSize(240, 160), 1.5);
        const QPixmap lowDpi = patternedImage(QSize(240, 160), 1.0);
        const QPixmap canvas = ScreenImage::compose(QRect(-240, 0, 480, 160),
            {{QRect(-240, 0, 240, 160), highDpi}, {QRect(0, 0, 240, 160), lowDpi}});
        QCOMPARE(canvas.size(), QSize(720, 240));
        QCOMPARE(canvas.devicePixelRatio(), 1.5);
        QCOMPARE(pixels(ScreenImage::crop(canvas, QRect(0, 0, 240, 160))), pixels(highDpi));
        QVERIFY(!ScreenImage::crop(canvas, QRect(240, 0, 240, 160)).isNull());
    }
};

QTEST_MAIN(ScreenImageTests)
#include "screenimage_tests.moc"
