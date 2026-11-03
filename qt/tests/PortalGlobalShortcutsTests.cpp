#include "GlobalShortcuts.h"
#include "PortalGlobalShortcuts.h"
#include <QApplication>
#include <QDBusVirtualObject>
#include <QDBusError>
#include <QDBusVariant>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
template<typename Predicate> static void waitFor(Predicate done, const char *message) {
    QElapsedTimer timer; timer.start();
    while (!done() && timer.elapsed() < 3000) { QApplication::processEvents(); QThread::msleep(1); }
    check(done(), message);
}
class TestPortal final : public QDBusVirtualObject {
public:
    QDBusConnection bus{QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("shortcut-test-service"))};
    QString session;
    uint result = 0;
    bool supported = true, holdResponse = false;
    int bindings = 0, closes = 0;
    QList<QDBusMessage> pending;
    QString introspect(const QString &) const override { return {}; }
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override {
        if (message.member() == QStringLiteral("Register")) {
            connection.send(message.createReply()); return true;
        }
        if (message.member() == QStringLiteral("Get")) {
            connection.send(supported ? message.createReply(QVariant::fromValue(QDBusVariant(uint(1))))
                : message.createErrorReply(QDBusError::UnknownInterface, QStringLiteral("no GlobalShortcuts portal")));
            return true;
        }
        if (message.member() == QStringLiteral("Close")) {
            ++closes; connection.send(message.createReply()); return true;
        }
        const bool create = message.member() == QStringLiteral("CreateSession");
        if (!create && message.member() != QStringLiteral("BindShortcuts")) return false;
        const auto options = qdbus_cast<QVariantMap>(message.arguments().last());
        QString sender = message.service().mid(1); sender.replace('.', '_');
        const auto request = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
            .arg(sender, options.value(QStringLiteral("handle_token")).toString());
        connection.send(message.createReply(QVariant::fromValue(QDBusObjectPath(request))));
        QVariantMap results;
        if (create) {
            session = QStringLiteral("/org/freedesktop/portal/desktop/session/%1/%2")
                .arg(sender, options.value(QStringLiteral("session_handle_token")).toString());
            results.insert(QStringLiteral("session_handle"), session);
        } else {
            ++bindings;
            const auto offered = qdbus_cast<PortalShortcutList>(message.arguments()[1]);
            check(offered.size() == 3 && offered[0].id == QStringLiteral("record")
                && offered[0].properties.value(QStringLiteral("preferred_trigger")) == QStringLiteral("CTRL+SHIFT+r"),
                "portal receives command IDs and XDG shortcut syntax");
            results.insert(QStringLiteral("shortcuts"), QVariant::fromValue(PortalShortcutList{
                {QStringLiteral("record"), {{QStringLiteral("trigger_description"), QStringLiteral("Super+F9")}}},
                {QStringLiteral("stop"), {{QStringLiteral("trigger_description"), QStringLiteral("Super+F10")}}}}));
        }
        auto reply = QDBusMessage::createSignal(request, QStringLiteral("org.freedesktop.portal.Request"), QStringLiteral("Response"));
        reply.setArguments({create ? uint(0) : result, results});
        if (holdResponse) pending.append(reply);
        else QTimer::singleShot(0, this, [this, reply] { bus.send(reply); });
        return true;
    }
    void key(const QString &member, const QString &id, const QString &path = {}) {
        auto message = QDBusMessage::createSignal(QStringLiteral("/org/freedesktop/portal/desktop"),
            QStringLiteral("org.freedesktop.portal.GlobalShortcuts"), member);
        message.setArguments({QVariant::fromValue(QDBusObjectPath(path.isEmpty() ? session : path)), id,
                              QVariant::fromValue(qulonglong(10)), QVariantMap{{QStringLiteral("activation_token"), QStringLiteral("portal-token")}}});
        bus.send(message);
    }
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QWidget window;
    TestPortal portal;
    const QString service = QStringLiteral("org.clipdock.TestShortcuts");
    check(portal.bus.isConnected() && portal.bus.registerService(service), "private test portal service registered");
    check(portal.bus.registerVirtualObject(QStringLiteral("/org/freedesktop/portal/desktop"), &portal,
                                          QDBusConnection::SubPath), "test portal methods registered");
    auto backend = std::make_unique<PortalGlobalShortcuts>(service);
    GlobalShortcuts shortcuts(&window, std::move(backend));
    std::array<int, 3> received{};
    QString receivedToken;
    QObject::connect(&shortcuts, &GlobalShortcuts::activated, &window,
        [&](ShortcutAction, const QString &token) { receivedToken = token; });
    QObject::connect(&shortcuts, &GlobalShortcuts::activated, &window, [&](ShortcutAction action) { ++received[int(action)]; });
    shortcuts.retry();
    waitFor([&] { return shortcuts.registeredCount() == 2; }, "approved portal subset becomes globally active");
    check(shortcuts.description().contains(QStringLiteral("Super+F9")), "portal-assigned trigger is shown to user");
    const auto firstSession = portal.session;
    portal.key(QStringLiteral("Activated"), QStringLiteral("record"));
    portal.key(QStringLiteral("Activated"), QStringLiteral("record"));
    portal.key(QStringLiteral("Activated"), QStringLiteral("pause"));
    waitFor([&] { return received[0] == 1; }, "portal activation delivered once");
    check(receivedToken == QStringLiteral("portal-token"), "portal window activation token reaches queued command");
    portal.key(QStringLiteral("Deactivated"), QStringLiteral("record"));
    portal.key(QStringLiteral("Activated"), QStringLiteral("record"));
    waitFor([&] { return received[0] == 2; }, "portal release enables next activation");
    check(received[1] == 0, "unapproved portal commands are ignored");
    portal.result = 1;
    shortcuts.retry();
    waitFor([&] { return shortcuts.description().contains(QStringLiteral("취소")); }, "denied portal request restores local fallback");
    check(shortcuts.registeredCount() == 0, "denied session retains no global registrations");
    portal.result = 0; portal.holdResponse = true;
    shortcuts.retry();
    waitFor([&] { return !portal.pending.isEmpty(); }, "delayed request captured");
    portal.holdResponse = false;
    shortcuts.retry();
    waitFor([&] { return shortcuts.registeredCount() == 2; }, "retry can establish a fresh session");
    const int bindCount = portal.bindings;
    for (const auto &reply : portal.pending) portal.bus.send(reply);
    portal.key(QStringLiteral("Activated"), QStringLiteral("stop"), firstSession);
    portal.key(QStringLiteral("Activated"), QStringLiteral("stop"));
    waitFor([&] { return received[2] == 1; }, "only current session delivers events");
    check(portal.bindings == bindCount, "late session responses cannot bind stale shortcuts");
    portal.supported = false;
    shortcuts.retry();
    waitFor([&] { return shortcuts.description().contains(QStringLiteral("지원하지")); }, "unsupported portal reports local-only operation");
    check(shortcuts.registeredCount() == 0 && portal.closes > 0, "sessions and pending requests are closed on retry");
    std::cout << "PASS: portal approval, rejection, triggers, repeats, stale replies and capability fallback\n";
}
