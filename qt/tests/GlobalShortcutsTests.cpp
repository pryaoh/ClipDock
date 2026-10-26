#include "GlobalShortcuts.h"
#include <QApplication>
#include <QShortcut>
#include <QWidget>
#include <cstdlib>
#include <iostream>

static void check(bool value, const char *message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
class ControlledBackend final : public ShortcutBackend {
public:
    void start() override {
        emit registrationChanged(ShortcutAction::Record, true, QStringLiteral("registered"));
        emit registrationChanged(ShortcutAction::Pause, false, QStringLiteral("conflict"));
        emit registrationChanged(ShortcutAction::Stop, true, QStringLiteral("registered"));
    }
    void stop() override {}
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QWidget window;
    auto backend = std::make_unique<ControlledBackend>();
    auto *source = backend.get();
    GlobalShortcuts shortcuts(&window, std::move(backend));
    std::array<int, 3> received{};
    QString receivedToken;
    QObject::connect(&shortcuts, &GlobalShortcuts::activated, &window,
        [&](ShortcutAction, const QString &token) { receivedToken = token; });
    QObject::connect(&shortcuts, &GlobalShortcuts::activated, &window, [&](ShortcutAction action) { ++received[int(action)]; });
    shortcuts.retry();
    check(shortcuts.registeredCount() == 2 && shortcuts.description().contains(QStringLiteral("conflict")),
          "partial registration reports conflict without losing other commands");
    for (int i = 0; i < 3; ++i) {
        auto *local = window.findChild<QShortcut *>(QStringLiteral("captureShortcut%1").arg(i));
        check(local && !local->autoRepeat() && local->isEnabled() == (i == 1),
              "only unregistered commands retain nonrepeating local fallback");
        local->activated();
    }
    check(received == std::array<int, 3>{0, 1, 0}, "local delivery cannot duplicate registered commands");
    source->keyStateChanged(ShortcutAction::Record, true, QStringLiteral("record-token"));
    source->keyStateChanged(ShortcutAction::Record, true);
    source->keyStateChanged(ShortcutAction::Pause, true);
    check(received[0] == 0, "native callback does not synchronously dispatch UI commands");
    app.processEvents();
    check(receivedToken == QStringLiteral("record-token"), "activation metadata survives queued delivery");
    check(received == std::array<int, 3>{1, 1, 0}, "held native key and unregistered native actions are ignored");
    source->keyStateChanged(ShortcutAction::Record, false);
    source->keyStateChanged(ShortcutAction::Record, true);
    app.processEvents();
    check(received[0] == 2, "physical release allows another action");
    source->keyStateChanged(ShortcutAction::Stop, true);
    shortcuts.retry();
    app.processEvents();
    check(received[2] == 0, "retry invalidates queued activations from previous registrations");
    source->registrationChanged(ShortcutAction::Record, false, QStringLiteral("lost registration"));
    window.findChild<QShortcut *>(QStringLiteral("captureShortcut0"))->activated();
    check(received[0] == 3, "registration loss restores local fallback");
    std::cout << "PASS: partial registration, local fallback, repeats, queued dispatch and retry\n";
}
