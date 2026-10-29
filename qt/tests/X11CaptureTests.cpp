#include "X11Capture.h"
#include <QGuiApplication>
#include <QScreen>
#include <X11/Xlib.h>
#include <iostream>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    auto *screen = app.primaryScreen();
    if (!screen || screen->devicePixelRatio() != 1.25 || !X11Capture::available()) return 2;
    Display *display = XOpenDisplay(nullptr);
    if (!display) return 2;
    XSetWindowAttributes attributes{};
    attributes.override_redirect = True;
    const Window window = XCreateWindow(display, DefaultRootWindow(display), 27, 0, 20, 20,
        0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);
    XMapWindow(display, window);
    XSync(display, False); // Server state changes without Qt receiving any GUI event.
    int failures = 0;
    auto check = [&](bool value, const char *message) {
        if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    };
    // 125%: [3,22) logical crops to [4,28) native. Pixel 27 belongs to the window.
    auto edge = X11Capture::grab(screen, QRect(3, 0, 19, 16), {window});
    check(edge.blocked && edge.image.isNull(), "fractional right-edge pixel must be excluded");
    auto full = X11Capture::grab(screen, {}, {window});
    check(full.blocked && full.image.isNull(), "mapped native window must block even before Qt events");
    auto outside = X11Capture::grab(screen, QRect(80, 80, 40, 40), {window});
    check(!outside.blocked && !outside.image.isNull(), "window outside capture region must not block");
    XUnmapWindow(display, window);
    XSync(display, False);
    auto hidden = X11Capture::grab(screen, {}, {window});
    check(!hidden.blocked && !hidden.image.isNull(), "unmapped window must allow a fresh image");
    XDestroyWindow(display, window);
    XSync(display, False);
    auto missing = X11Capture::grab(screen, {}, {window});
    check(missing.blocked && missing.image.isNull(), "missing native window must fail closed");
    auto afterError = X11Capture::grab(screen, {}, {});
    check(!afterError.image.isNull(), "server grab must be released after a query error");
    XCloseDisplay(display);
    if (!failures) std::cout << "PASS: native visibility, fractional bounds, nonoverlap and error cleanup\n";
    return failures ? 1 : 0;
}
