#include "ShortcutBackend.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <cstdlib>
#include <iostream>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    constexpr int occupiedId = 0x1ad0;
    check(RegisterHotKey(nullptr, occupiedId, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'R'),
          "test can reserve record key to create a real registration conflict");
    auto backend = createShortcutBackend();
    std::array<bool, 3> registered{};
    std::array<int, 3> received{};
    QObject::connect(backend.get(), &ShortcutBackend::registrationChanged, &app,
                     [&](ShortcutAction action, bool ok, const QString &) { registered[int(action)] = ok; });
    QObject::connect(backend.get(), &ShortcutBackend::keyStateChanged, &app,
                     [&](ShortcutAction action, bool pressed) { if (pressed) ++received[int(action)]; });
    backend->start();
    check(registered == std::array<bool, 3>{false, true, true}, "native registration isolates a conflicting key");
    auto receiver = FindWindowExW(HWND_MESSAGE, nullptr, L"ClipDock.GlobalShortcuts", nullptr);
    check(receiver, "native message receiver exists without a visible app window");
    PostMessageW(receiver, WM_HOTKEY, 2, MAKELPARAM(MOD_CONTROL | MOD_SHIFT, 'P'));
    PostMessageW(receiver, WM_HOTKEY, 3, MAKELPARAM(MOD_CONTROL | MOD_SHIFT, 'S'));
    PostMessageW(receiver, WM_HOTKEY, 1, MAKELPARAM(MOD_CONTROL | MOD_SHIFT, 'R'));
    QElapsedTimer timer;
    timer.start();
    while (received[2] == 0 && timer.elapsed() < 2000) { app.processEvents(); QThread::msleep(1); }
    check(received == std::array<int, 3>{0, 1, 1}, "OS messages deliver registered commands only");
    backend->stop();
    check(!FindWindowExW(HWND_MESSAGE, nullptr, L"ClipDock.GlobalShortcuts", nullptr), "stop destroys native receiver");
    check(RegisterHotKey(nullptr, occupiedId + 1, MOD_CONTROL | MOD_SHIFT, 'P'), "stop releases pause registration");
    check(RegisterHotKey(nullptr, occupiedId + 2, MOD_CONTROL | MOD_SHIFT, 'S'), "stop releases stop registration");
    check(!RegisterHotKey(nullptr, occupiedId + 3, MOD_CONTROL | MOD_SHIFT, 'R'), "stop preserves another owner's conflict");
    UnregisterHotKey(nullptr, occupiedId);
    UnregisterHotKey(nullptr, occupiedId + 1);
    UnregisterHotKey(nullptr, occupiedId + 2);
    backend->start();
    check(registered == std::array<bool, 3>{true, true, true}, "retry registers all keys after conflict is removed");
    backend.reset();
    check(RegisterHotKey(nullptr, occupiedId, MOD_CONTROL | MOD_SHIFT, 'R'), "destructor releases registrations");
    UnregisterHotKey(nullptr, occupiedId);
#endif
    std::cout << "PASS: native registration, conflict isolation, delivery, retry and release\n";
}
