#pragma once

#include <QCapturableWindow>
#include <QList>
#include <QString>
#include <QRectF>

class QScreen;

struct NativeWindowCandidate {
    quint64 id = 0;
    QString title;
    QRectF bounds; // Native desktop coordinates, in front-to-back stacking order.
};

bool nativeWindowPickingSupported();
QList<NativeWindowCandidate> nativePickerWindows();
NativeWindowCandidate pickerWindowAt(const QList<NativeWindowCandidate> &windows, const QPointF &nativePoint);
QPointF pickerNativePoint(const QPointF &globalPosition, const QRectF &screen, qreal scale);
QRectF pickerOverlayRect(const QRectF &nativeBounds, const QRectF &screen, qreal scale);
qreal pickerNativeScale(const QScreen *screen);
bool pickerMacWindowEligible(const QString &owner, int layer, double alpha, qint64 pid, qint64 ownPid);
QCapturableWindow capturableWindowWithId(const QList<QCapturableWindow> &windows, quint64 id);
