#pragma once
#include <QImage>
#include <QRect>
#include <QSize>
#include <QtMath>

// Region coordinates are screen-local logical pixels. Derive the scale from
// each captured image, not devicePixelRatio (capture backends can scale frames).
inline QRect regionPixelRect(const QRect &region, const QSize &screenSize, const QSize &frameSize) {
    if (screenSize.isEmpty() || frameSize.isEmpty() || region.width() < 16 || region.height() < 16
        || !QRect(QPoint(), screenSize).contains(region)) return {};
    const double sx = double(frameSize.width()) / screenSize.width();
    const double sy = double(frameSize.height()) / screenSize.height();
    const int left = qRound(region.x() * sx), top = qRound(region.y() * sy);
    const int right = qRound((region.x() + region.width()) * sx);
    const int bottom = qRound((region.y() + region.height()) * sy);
    return QRect(left, top, right - left, bottom - top);
}

inline QImage cropRegionImage(const QImage &image, const QRect &region, const QSize &screenSize) {
    const auto pixels = regionPixelRect(region, screenSize, image.size());
    if (pixels.isEmpty()) return {};
    return image.copy(pixels).convertToFormat(QImage::Format_ARGB32);
}
