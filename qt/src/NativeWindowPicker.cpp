#include "NativeWindowPicker.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
// Qt 6.8 has no public native window ID API. Keep the version-sensitive bridge
// in this file and use the headers from the same Qt kit as the linked library.
// Matching titles would silently select the wrong window when titles repeat.
#include <QtMultimedia/private/qcapturablewindow_p.h>
#endif

#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#elif defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

bool nativeWindowPickingSupported() {
#if defined(Q_OS_MACOS)
    return QGuiApplication::platformName() == QStringLiteral("cocoa");
#elif defined(Q_OS_WIN)
    return QGuiApplication::platformName() == QStringLiteral("windows");
#else
    return false;
#endif
}

QCapturableWindow capturableWindowWithId(const QList<QCapturableWindow> &windows, quint64 id) {
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    if (id == 0) return {};
    for (const auto &window : windows) {
        const auto *handle = QCapturableWindowPrivate::handle(window);
        // The caller validates liveness immediately before accepting the selection.
        if (handle && handle->id == id) return window;
    }
#else
    Q_UNUSED(windows);
    Q_UNUSED(id);
#endif
    return {};
}

QPointF pickerNativePoint(const QPointF &globalPosition, const QRectF &screen, qreal scale) {
    return screen.topLeft() + (globalPosition - screen.topLeft()) * scale;
}

QRectF pickerOverlayRect(const QRectF &nativeBounds, const QRectF &screen, qreal scale) {
    return QRectF((nativeBounds.topLeft() - screen.topLeft()) / scale,
                  nativeBounds.size() / scale);
}

qreal pickerNativeScale(const QScreen *screen) {
#ifdef Q_OS_WIN
    return screen->devicePixelRatio();
#else
    // Quartz window bounds and Qt macOS screen geometry both use logical points.
    Q_UNUSED(screen);
    return 1;
#endif
}

NativeWindowCandidate pickerWindowAt(const QList<NativeWindowCandidate> &windows, const QPointF &point) {
    for (const auto &window : windows) {
        // QRectF::contains includes the far edge. Desktop hit-testing is half-open.
        if (point.x() >= window.bounds.left() && point.x() < window.bounds.right()
            && point.y() >= window.bounds.top() && point.y() < window.bounds.bottom()) return window;
    }
    return {};
}

bool pickerMacWindowEligible(const QString &owner, int layer, double alpha, qint64 pid, qint64 ownPid) {
    // Dock creates large transparent surfaces for Stage Manager/Mission Control.
    // They can have alpha=1 and appear in Qt's list despite not being app windows.
    return pid != ownPid && layer == 0 && alpha > 0
        && owner != QStringLiteral("Dock") && owner != QStringLiteral("Window Server")
        && owner != QStringLiteral("WindowServer") && owner != QStringLiteral("SystemUIServer")
        && owner != QStringLiteral("Control Center") && owner != QStringLiteral("ControlCenter")
        && owner != QStringLiteral("Wallpaper");
}

QList<NativeWindowCandidate> nativePickerWindows() {
    QList<NativeWindowCandidate> result;
#ifdef Q_OS_MACOS
    const auto windows = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!windows) return result;
    for (CFIndex i = 0; i < CFArrayGetCount(windows); ++i) {
        const auto info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows, i));
        int pid = 0, layer = 0;
        const auto pidValue = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowOwnerPID));
        const auto layerValue = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowLayer));
        if (!pidValue || !layerValue) continue;
        CFNumberGetValue(pidValue, kCFNumberIntType, &pid);
        CFNumberGetValue(layerValue, kCFNumberIntType, &layer);
        double alpha = 1;
        const auto alphaValue = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowAlpha));
        if (alphaValue) CFNumberGetValue(alphaValue, kCFNumberDoubleType, &alpha);
        const auto ownerValue = static_cast<CFStringRef>(CFDictionaryGetValue(info, kCGWindowOwnerName));
        const auto owner = ownerValue ? QString::fromCFString(ownerValue) : QString();
        if (!pickerMacWindowEligible(owner, layer, alpha, pid, QCoreApplication::applicationPid())) continue;
        CGRect bounds;
        const auto boundsValue = static_cast<CFDictionaryRef>(CFDictionaryGetValue(info, kCGWindowBounds));
        if (!boundsValue || !CGRectMakeWithDictionaryRepresentation(boundsValue, &bounds)
            || CGRectIsEmpty(bounds)) continue;
        const auto number = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowNumber));
        int64_t id = 0;
        if (!number || !CFNumberGetValue(number, kCFNumberSInt64Type, &id)) continue;
        NativeWindowCandidate candidate;
        candidate.id = static_cast<quint64>(id);
        candidate.bounds = QRectF(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
        const auto name = static_cast<CFStringRef>(CFDictionaryGetValue(info, kCGWindowName));
        if (name) candidate.title = QString::fromCFString(name);
        if (candidate.title.isEmpty()) candidate.title = owner;
        result.append(candidate);
    }
    CFRelease(windows);
#elif defined(Q_OS_WIN)
    EnumWindows([](HWND window, LPARAM data) -> BOOL {
        auto &result = *reinterpret_cast<QList<NativeWindowCandidate> *>(data);
        DWORD pid = 0;
        GetWindowThreadProcessId(window, &pid);
        if (pid == GetCurrentProcessId() || !IsWindowVisible(window) || IsIconic(window)
            || window == GetShellWindow()) return TRUE;
        RECT bounds;
        if (!GetWindowRect(window, &bounds) || IsRectEmpty(&bounds)) return TRUE;
        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
            && cloaked) return TRUE;
        // Use visible frame bounds instead of invisible Windows resize margins.
        RECT visible;
        if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &visible, sizeof(visible))))
            bounds = visible;
        NativeWindowCandidate candidate;
        candidate.id = reinterpret_cast<quintptr>(window);
        candidate.bounds = QRectF(bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top);
        wchar_t title[1024]{};
        GetWindowTextW(window, title, 1024);
        candidate.title = QString::fromWCharArray(title);
        result.append(candidate);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&result));
#endif
    return result;
}
