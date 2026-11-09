#include "maccapture.h"

#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <QCoreApplication>
#include <QMetaObject>


namespace {
void finish(MacCaptureCallback callback, QVector<MacCaptureResult> results, QString error, bool permissionDenied = false) {
    QMetaObject::invokeMethod(QCoreApplication::instance(),
        [callback, results, error, permissionDenied] { callback(results, error, permissionDenied); }, Qt::QueuedConnection);
}

void captureNext(SCShareableContent *content, QVector<MacCaptureRequest> requests,
                 int index, QVector<MacCaptureResult> results, MacCaptureCallback callback) API_AVAILABLE(macos(14.0));
void captureNext(SCShareableContent *content, QVector<MacCaptureRequest> requests,
                 int index, QVector<MacCaptureResult> results, MacCaptureCallback callback) {
    if (index == requests.size()) {
        finish(callback, results, {});
        return;
    }
    const auto request = requests[index];
    SCDisplay *display = nil;
    for (SCDisplay *candidate in content.displays) {
        if (candidate.displayID == request.displayId) { display = candidate; break; }
    }
    if (!display) {
        finish(callback, {}, QStringLiteral("显示器已断开，请重试。"));
        return;
    }
    SCContentFilter *filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
    SCStreamConfiguration *configuration = [SCStreamConfiguration new];
    configuration.width = qRound(request.geometry.width() * request.ratio);
    configuration.height = qRound(request.geometry.height() * request.ratio);
    configuration.showsCursor = NO;
    [SCScreenshotManager captureImageWithFilter:filter configuration:configuration
        completionHandler:^(CGImageRef image, NSError *error) {
        if (!image || error) {
            finish(callback, {}, error ? QString::fromUtf8(error.localizedDescription.UTF8String)
                                      : QStringLiteral("无法读取屏幕图像。"), error.code == SCStreamErrorUserDeclined);
            return;
        }
        QImage pixels(int(CGImageGetWidth(image)), int(CGImageGetHeight(image)), QImage::Format_ARGB32_Premultiplied);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(pixels.bits(), pixels.width(), pixels.height(), 8,
            pixels.bytesPerLine(), colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
        CGColorSpaceRelease(colorSpace);
        if (!context) { finish(callback, {}, QStringLiteral("无法创建截图缓冲区。")); return; }
        CGContextDrawImage(context, CGRectMake(0, 0, pixels.width(), pixels.height()), image);
        CGContextRelease(context);
        pixels.setDevicePixelRatio(request.ratio);
        auto next = results;
        next.append({request.geometry, pixels});
        captureNext(content, requests, index + 1, next, callback);
    }];
}
}

void captureMacScreens(const QVector<MacCaptureRequest> &requests, MacCaptureCallback callback) {
    if (@available(macOS 14.0, *)) {
        // ScreenCaptureKit performs its own authorization. A stale legacy
        // CGPreflightScreenCaptureAccess result must not block a valid capture.
        [SCShareableContent getShareableContentExcludingDesktopWindows:NO onScreenWindowsOnly:YES
            completionHandler:^(SCShareableContent *content, NSError *error) {
            if (!content || error) {
                finish(callback, {}, error ? QString::fromUtf8(error.localizedDescription.UTF8String)
                                          : QStringLiteral("无法访问屏幕内容。"), error.code == SCStreamErrorUserDeclined);
                return;
            }
            captureNext(content, requests, 0, {}, callback);
        }];
    } else {
        finish(callback, {}, QStringLiteral("ScreenCaptureKit 截图需要 macOS 14。"));
    }
}
