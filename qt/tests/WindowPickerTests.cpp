#include "WindowPicker.h"
#include "NativeWindowPicker.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QWidget>
#include <iostream>
#include <cstdlib>

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
#include <QtMultimedia/private/qcapturablewindow_p.h>
#endif

static void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // System surfaces must not obscure app windows in the picker (Dock may be layer 0).
    check(!pickerMacWindowEligible(QStringLiteral("Dock"), 0, 1, 12, 99), "ignore Dock layer-zero surface");
    check(!pickerMacWindowEligible(QStringLiteral("SystemUIServer"), 0, 1, 12, 99), "ignore system UI");
    check(!pickerMacWindowEligible(QStringLiteral("App"), 0, 1, 99, 99), "ignore own overlay");
    check(!pickerMacWindowEligible(QStringLiteral("App"), 0, 0, 12, 99), "ignore invisible window");
    check(!pickerMacWindowEligible(QStringLiteral("App"), 24, 1, 12, 99), "ignore menu layer");
    check(pickerMacWindowEligible(QStringLiteral("App"), 0, 1, 12, 99), "accept normal app window");

    const NativeWindowCandidate front{11, QStringLiteral("Same"), QRectF(-400, -200, 300, 200)};
    const NativeWindowCandidate back{22, QStringLiteral("Same"), QRectF(-500, -300, 800, 600)};
    check(pickerWindowAt({front, back}, QPointF(-250, -100)).id == 11, "front window wins overlap");
    check(pickerWindowAt({front, back}, QPointF(10, 10)).id == 22, "exposed background window");
    check(pickerWindowAt({front, back}, QPointF(-100, -100)).id == 22, "right edge belongs to adjacent area");
    check(pickerWindowAt({front, back}, QPointF(-250, 0)).id == 22, "bottom edge belongs to adjacent area");
    check(pickerWindowAt({front, back}, QPointF(900, 900)).id == 0, "desktop has no target");
    check(pickerWindowAt({back}, QPointF(-250, -100)).id == 22, "snapshot after front window closes");

    const QRectF leftScreen(-1920, -200, 1920, 1080);
    check(pickerNativePoint(QPointF(-500, 200), leftScreen, 1) == QPointF(-500, 200), "macOS points are not scaled again on Retina");
    check(pickerOverlayRect(QRectF(-1800, -100, 800, 600), leftScreen, 1)
        == QRectF(120, 100, 800, 600), "negative monitor origin highlight");
    const QRectF scaledScreen(1920, 0, 1280, 720);
    check(pickerNativePoint(QPointF(2020, 100), scaledScreen, 2) == QPointF(2120, 200), "Windows DPI scaling about monitor origin");
    check(pickerOverlayRect(QRectF(2120, 200, 800, 600), scaledScreen, 2)
        == QRectF(100, 100, 400, 300), "Windows highlight uses logical pixels");
    check(pickerOverlayRect(QRectF(1700, 100, 1000, 500), scaledScreen, 2)
        == QRectF(-110, 50, 500, 250), "spanning window retains real frame before clipping");

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    // Duplicate/empty titles and changed enumeration order must never affect identity.
    auto makeWindow = [](size_t id, const QString &title) {
        auto *data = new QCapturableWindowPrivate;
        data->id = id;
        data->description = title;
        return data->create();
    };
    const auto first = makeWindow(101, QStringLiteral("Same title"));
    const auto second = makeWindow(202, QStringLiteral("Same title"));
    const auto untitled = makeWindow(303, {});
    check(capturableWindowWithId({first, second, untitled}, 202) == second, "duplicate titles");
    check(capturableWindowWithId({untitled, second, first}, 101) == first, "reordered list");
    check(capturableWindowWithId({first, untitled}, 303) == untitled, "untitled window");
    check(capturableWindowWithId({first}, 202) == QCapturableWindow(), "closed window must not select another");
    check(capturableWindowWithId({first}, 0) == QCapturableWindow(), "desktop is not a window");
#endif

    // Exercise overlay event routing and deferred cleanup without screen capture permissions.
    for (int cancelMethod = 0; cancelMethod < 3; ++cancelMethod) {
        int finishedCount = 0;
        QPointer<WindowPicker> picker = new WindowPicker;
        QObject::connect(picker, &WindowPicker::finished, &app, [&](const QCapturableWindow &window) {
            ++finishedCount;
            check(window == QCapturableWindow(), "cancellation must not change target");
        });
        picker->start();
        QWidget *overlay = nullptr;
        for (auto *widget : QApplication::topLevelWidgets())
            if (widget->isVisible() && widget->windowTitle() == QStringLiteral("녹화할 창 선택")) overlay = widget;
        check(overlay, "picker overlay is visible");
        QMouseEvent strayRelease(QEvent::MouseButtonRelease, QPointF(10, 10), QPointF(10, 10),
                                 Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(overlay, &strayRelease);
        check(finishedCount == 0, "release without a press must not select a window");
        if (cancelMethod == 0) {
            QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QApplication::sendEvent(overlay, &escape);
            QApplication::sendEvent(overlay, &escape);
        } else if (cancelMethod == 1) {
            QMouseEvent rightClick(QEvent::MouseButtonRelease, QPointF(10, 10), QPointF(10, 10),
                                  Qt::RightButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(overlay, &rightClick);
        } else {
            overlay->close();
        }
        check(finishedCount == 1, "cancel emits once");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        check(picker.isNull(), "picker is deleted after cancellation");
        for (auto *widget : QApplication::topLevelWidgets())
            check(widget->windowTitle() != QStringLiteral("녹화할 창 선택"), "no stale overlay");
    }
    std::cout << "PASS: system-window filtering, stacking, monitor/DPI coordinates, window identity, cancellation, cleanup\n";
    return 0;
}
