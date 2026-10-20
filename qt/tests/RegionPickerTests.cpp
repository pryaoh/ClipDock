#include "RegionCapture.h"
#include "RegionPicker.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QScreen>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
static void mouse(QWidget *widget, QEvent::Type type, QPointF p, Qt::MouseButton button) {
    QMouseEvent event(type, p, widget->mapToGlobal(p.toPoint()), button,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : button, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    check(regionPixelRect({100, 50, 320, 180}, {1000, 600}, {2000, 1200}) == QRect(200, 100, 640, 360), "Retina crop");
    check(regionPixelRect({20, 40, 200, 100}, {800, 600}, {1000, 750}) == QRect(25, 50, 250, 125), "fractional scale");
    check(regionPixelRect({900, 500, 100, 100}, {1000, 600}, {1500, 900}) == QRect(1350, 750, 150, 150), "bottom right bounds");
    check(regionPixelRect({-1, 0, 50, 50}, {100, 100}, {200, 200}).isEmpty(), "reject negative local bounds");
    check(regionPixelRect({50, 50, 100, 100}, {100, 100}, {200, 200}).isEmpty(), "reject outside bounds");
    check(regionPixelRect({0, 0, 2, 50}, {100, 100}, {200, 200}).isEmpty(), "reject tiny region");
    QImage image(200, 200, QImage::Format_ARGB32);
    image.fill(Qt::red);
    for (int y = 40; y < 120; ++y) for (int x = 20; x < 100; ++x) image.setPixelColor(x, y, Qt::green);
    const auto cropped = cropRegionImage(image, {10, 20, 40, 40}, {100, 100});
    check(cropped.size() == QSize(80, 80), "cropped size");
    for (int y = 0; y < 80; ++y) for (int x = 0; x < 80; ++x)
        check(cropped.pixelColor(x, y) == QColor(Qt::green), "no pixels outside selection");

    for (int mode = 0; mode < 6; ++mode) {
        QPointer<RegionPicker> picker = new RegionPicker(app.primaryScreen());
        int finished = 0;
        QRect selected;
        QObject::connect(picker, &RegionPicker::finished, &app, [&](const QRect &r) { ++finished; selected = r; });
        picker->start();
        mouse(picker, QEvent::MouseButtonRelease, {150, 120}, Qt::LeftButton);
        check(finished == 0, "ignore release without press");
        mouse(picker, QEvent::MouseButtonPress, {10, 10}, Qt::LeftButton);
        mouse(picker, QEvent::MouseButtonRelease, {12, 12}, Qt::LeftButton);
        check(finished == 0, "tiny drag stays open");
        if (mode < 2) {
            const QPointF start = mode ? QPointF(180, 140) : QPointF(20, 30);
            const QPointF end = mode ? QPointF(20, 30) : QPointF(180, 140);
            mouse(picker, QEvent::MouseButtonPress, start, Qt::LeftButton);
            mouse(picker, QEvent::MouseMove, end, Qt::LeftButton);
            mouse(picker, QEvent::MouseButtonRelease, end, Qt::LeftButton);
            check(selected == QRect(20, 30, 160, 110), "forward and reverse drag identical");
        } else if (mode == 2) {
            mouse(picker, QEvent::MouseButtonPress, {20, 30}, Qt::LeftButton);
            mouse(picker, QEvent::MouseButtonRelease, {10000, 10000}, Qt::LeftButton);
            check(selected == QRect(20, 30, picker->width() - 20, picker->height() - 30), "clip drag to selected monitor");
        } else if (mode == 3) {
            QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QApplication::sendEvent(picker, &escape);
        } else if (mode == 4) mouse(picker, QEvent::MouseButtonPress, {10, 10}, Qt::RightButton);
        else picker->close();
        check(finished == 1, "one result per selection");
        if (mode >= 3) check(selected.isEmpty(), "cancel has no selection");
        check(!picker->isVisible(), "overlay hides before capture");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        check(!picker, "overlay cleaned up");
    }
    std::cout << "PASS: region pixel mapping, crop isolation, drag directions, bounds, cancellation, cleanup\n";
}
