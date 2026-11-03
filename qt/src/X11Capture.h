#pragma once
#include <QImage>
#include <QList>
#include <QRect>
#include <QtGui/qwindowdefs.h>

class QScreen;

namespace X11Capture {
bool available();
struct Frame {
    QImage image;
    bool blocked = false;
};
// Native visibility/geometry and pixel acquisition share one X server transaction.
// A blocked frame is deliberately omitted; a null unblocked image is an error.
Frame grab(QScreen *screen, const QRect &region, const QList<WId> &excluded);
}
