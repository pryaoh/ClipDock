#include "GlobalShortcuts.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <QWidget>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
static void drain() {
    QElapsedTimer timer; timer.start();
    while (timer.elapsed() < 100) { QApplication::processEvents(); QThread::msleep(1); }
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto *other = XOpenDisplay(nullptr);
    check(other, "Xvfb display opened");
    const auto root = DefaultRootWindow(other);
    const auto r = XKeysymToKeycode(other, XK_r);
    XGrabKey(other, r, ControlMask | ShiftMask, root, False, GrabModeAsync, GrabModeAsync);
    XSync(other, False);
    auto focus = XCreateSimpleWindow(other, root, 0, 0, 200, 100, 0, 0, 0);
    XMapWindow(other, focus);
    XSetInputFocus(other, focus, RevertToParent, CurrentTime);
    XSync(other, False);
    QWidget window;
    int pauses = 0, stops = 0;
    {
        GlobalShortcuts shortcuts(&window);
        QObject::connect(&shortcuts, &GlobalShortcuts::activated, &window, [&](ShortcutAction action) {
            if (action == ShortcutAction::Pause) ++pauses;
            if (action == ShortcutAction::Stop) ++stops;
        });
        shortcuts.retry();
        check(shortcuts.registeredCount() == 2, "X11 isolates an existing passive grab conflict");
        auto send = [&](KeySym key, bool down) { XTestFakeKeyEvent(other, XKeysymToKeycode(other, key), down, CurrentTime); };
        send(XK_Control_L, true); send(XK_Shift_L, true); send(XK_p, true); send(XK_p, true);
        XSync(other, False); drain();
        check(pauses == 1, "foreign focused window delivers exactly one global pause while held");
        send(XK_p, false); send(XK_p, true); send(XK_p, false);
        send(XK_Shift_L, false); send(XK_Control_L, false);
        XSync(other, False); drain();
        check(pauses == 2, "X11 release allows another activation");
        send(XK_Caps_Lock, true); send(XK_Caps_Lock, false);
        send(XK_Control_L, true); send(XK_Shift_L, true); send(XK_p, true); send(XK_p, false);
        send(XK_Shift_L, false); send(XK_Control_L, false);
        XSync(other, False); drain();
        check(pauses == 3, "Caps Lock does not disable registered shortcuts");
        send(XK_Control_L, true); send(XK_Shift_L, true); send(XK_p, true);
        send(XK_Shift_L, false); send(XK_Control_L, false);
        send(XK_s, true); send(XK_s, false); send(XK_p, false);
        XSync(other, False); drain();
        check(stops == 0, "unmodified keys during an active grab cannot trigger commands");
        send(XK_Control_L, true); send(XK_Shift_L, true);
        send(XK_p, true); send(XK_s, true); send(XK_p, false); send(XK_s, false);
        XSync(other, False); drain();
        check(stops == 0, "overlapping chords wait for the first trigger to be released");
        send(XK_s, true); send(XK_s, false);
        send(XK_Shift_L, false); send(XK_Control_L, false);
        XSync(other, False); drain();
        check(stops == 1, "an ignored overlapping key does not block its next independent press");
    }
    // A second backend can now claim P/S, while the foreign R grab stays intact.
    auto backend = createShortcutBackend();
    int registered = 0;
    QObject::connect(backend.get(), &ShortcutBackend::registrationChanged, &window,
        [&](ShortcutAction, bool ok, const QString &) { if (ok) ++registered; });
    backend->start();
    check(registered == 2, "destruction releases owned grabs without removing another client's grab");
    backend->stop();
    XUngrabKey(other, r, ControlMask | ShiftMask, root);
    XDestroyWindow(other, focus);
    XCloseDisplay(other);
    std::cout << "PASS: X11 foreign-focus activation, conflicts, repeats, locks and cleanup\n";
}
