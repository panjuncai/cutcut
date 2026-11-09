#pragma once

#include "screenimage.h"
#include <QImage>
#include <functional>

struct MacCaptureRequest {
    unsigned int displayId;
    QRect geometry;
    qreal ratio;
};
struct MacCaptureResult {
    QRect geometry;
    QImage image;
};
using MacCaptureCallback = std::function<void(QVector<MacCaptureResult>, QString, bool)>;
// Completion runs on the GUI thread. No QPixmap is created on capture threads.
void captureMacScreens(const QVector<MacCaptureRequest> &requests, MacCaptureCallback callback);
