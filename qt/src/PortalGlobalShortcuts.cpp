#include "PortalGlobalShortcuts.h"
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QUuid>
#include <algorithm>

namespace {
const QString desktop = QStringLiteral("/org/freedesktop/portal/desktop");
const QString shortcutsInterface = QStringLiteral("org.freedesktop.portal.GlobalShortcuts");
const QString requestInterface = QStringLiteral("org.freedesktop.portal.Request");
const QString sessionInterface = QStringLiteral("org.freedesktop.portal.Session");
const QStringList ids {QStringLiteral("record"), QStringLiteral("pause"), QStringLiteral("stop")};
QString token() { return QStringLiteral("clipdock") + QUuid::createUuid().toString(QUuid::Id128); }
}

QDBusArgument &operator<<(QDBusArgument &argument, const PortalShortcut &shortcut) {
    argument.beginStructure(); argument << shortcut.id << shortcut.properties; argument.endStructure(); return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, PortalShortcut &shortcut) {
    argument.beginStructure(); argument >> shortcut.id >> shortcut.properties; argument.endStructure(); return argument;
}
PortalGlobalShortcuts::PortalGlobalShortcuts(const QString &service)
    : m_connectionName(token()),
      m_bus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_connectionName)), m_service(service) {
    qDBusRegisterMetaType<PortalShortcut>();
    qDBusRegisterMetaType<PortalShortcutList>();
    m_bus.connect(m_service, QString(), requestInterface, QStringLiteral("Response"),
                  this, SLOT(response(uint,QVariantMap,QDBusMessage)));
    m_bus.connect(m_service, desktop, shortcutsInterface, QStringLiteral("Activated"),
                  this, SLOT(activated(QDBusObjectPath,QString,qulonglong,QVariantMap)));
    m_bus.connect(m_service, desktop, shortcutsInterface, QStringLiteral("Deactivated"),
                  this, SLOT(deactivated(QDBusObjectPath,QString,qulonglong,QVariantMap)));
    m_bus.connect(m_service, desktop, shortcutsInterface, QStringLiteral("ShortcutsChanged"),
                  this, SLOT(changed(QDBusObjectPath,PortalShortcutList)));
    m_bus.connect(m_service, QString(), sessionInterface, QStringLiteral("Closed"),
                  this, SLOT(closed(QVariantMap,QDBusMessage)));
    auto *watcher = new QDBusServiceWatcher(m_service, m_bus, QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] {
        if (m_started) fail(QStringLiteral("시스템 단축키 서비스가 종료되었습니다. 다시 등록하세요."));
    });
}
PortalGlobalShortcuts::~PortalGlobalShortcuts() {
    stop();
    QDBusConnection::disconnectFromBus(m_connectionName);
}
void PortalGlobalShortcuts::start() {
    stop();
    m_started = true;
    if (!m_bus.isConnected()) { fail(QStringLiteral("세션 D-Bus에 연결할 수 없습니다.")); return; }
    const auto generation = m_generation;
    // Give an unsandboxed app its identity before making other portal calls.
    // Old portals have no Registry; sandboxed/already registered peers reject it.
    auto message = QDBusMessage::createMethodCall(m_service, desktop,
        QStringLiteral("org.freedesktop.host.portal.Registry"), QStringLiteral("Register"));
    message.setArguments({QStringLiteral("io.github.pryaoh.ClipDock"), QVariantMap{}});
    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, generation](QDBusPendingCallWatcher *done) {
        done->deleteLater();
        if (generation == m_generation) checkCapability(generation);
    });
}

void PortalGlobalShortcuts::checkCapability(unsigned generation) {
    auto message = QDBusMessage::createMethodCall(m_service, desktop,
        QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"));
    message.setArguments({shortcutsInterface, QStringLiteral("version")});
    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, generation](QDBusPendingCallWatcher *done) {
        QDBusPendingReply<QDBusVariant> reply = *done;
        done->deleteLater();
        if (generation != m_generation) return;
        if (reply.isError() || reply.value().variant().toUInt() < 1) {
            fail(QStringLiteral("이 데스크톱은 GlobalShortcuts 포털을 지원하지 않거나 서비스에 연결할 수 없습니다."));
            return;
        }
        request(true);
    });
}

void PortalGlobalShortcuts::request(bool create) {
    m_creating = create;
    auto sender = m_bus.baseService().mid(1); sender.replace('.', '_');
    const auto requestToken = token();
    m_request = desktop + QStringLiteral("/request/%1/%2").arg(sender, requestToken);
    QVariantMap options{{QStringLiteral("handle_token"), requestToken}};
    auto message = QDBusMessage::createMethodCall(m_service, desktop, shortcutsInterface,
        create ? QStringLiteral("CreateSession") : QStringLiteral("BindShortcuts"));
    if (create) {
        const auto sessionToken = token();
        options.insert(QStringLiteral("session_handle_token"), sessionToken);
        m_session = desktop + QStringLiteral("/session/%1/%2").arg(sender, sessionToken);
        message.setArguments({options});
    } else {
        PortalShortcutList shortcuts;
        const QStringList triggers{QStringLiteral("CTRL+SHIFT+r"), QStringLiteral("CTRL+SHIFT+p"), QStringLiteral("CTRL+SHIFT+s")};
        for (auto action : shortcutActions)
            shortcuts.append({ids[int(action)], {{QStringLiteral("description"), shortcutName(action)},
                              {QStringLiteral("preferred_trigger"), triggers[int(action)]}}});
        message.setArguments({QVariant::fromValue(QDBusObjectPath(m_session)), QVariant::fromValue(shortcuts), QString(), options});
        for (auto action : shortcutActions)
            emit registrationChanged(action, false, QStringLiteral("시스템 단축키 승인 창에서 설정을 확인하세요."));
    }
    const auto generation = m_generation;
    const auto expected = m_request;
    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, generation, expected](QDBusPendingCallWatcher *done) {
        QDBusPendingReply<QDBusObjectPath> reply = *done;
        done->deleteLater();
        if (generation != m_generation || m_request != expected) return;
        if (reply.isError()) { fail(QStringLiteral("전역 단축키 등록 실패: %1").arg(reply.error().message())); return; }
        m_request = reply.value().path();
    });
}

void PortalGlobalShortcuts::stop() {
    ++m_generation;
    m_started = false;
    if (!m_request.isEmpty())
        m_bus.call(QDBusMessage::createMethodCall(m_service, m_request, requestInterface, QStringLiteral("Close")), QDBus::NoBlock);
    if (!m_session.isEmpty())
        m_bus.call(QDBusMessage::createMethodCall(m_service, m_session, sessionInterface, QStringLiteral("Close")), QDBus::NoBlock);
    m_request.clear();
    m_session.clear();
}

void PortalGlobalShortcuts::fail(const QString &message) {
    stop();
    for (auto action : shortcutActions) emit registrationChanged(action, false, message);
}

void PortalGlobalShortcuts::response(uint code, const QVariantMap &results, const QDBusMessage &message) {
    if (message.path() != m_request || !m_started) {
        // A cancelled CreateSession may finish late: close it, never bind it.
        const auto stale = results.value(QStringLiteral("session_handle")).toString();
        if (stale.startsWith(desktop + QStringLiteral("/session/")) && stale != m_session)
            m_bus.call(QDBusMessage::createMethodCall(m_service, stale, sessionInterface, QStringLiteral("Close")), QDBus::NoBlock);
        return;
    }
    m_request.clear();
    if (code != 0) {
        fail(code == 1 ? QStringLiteral("시스템 승인 창에서 등록을 취소했습니다.")
                       : QStringLiteral("시스템에서 단축키 등록을 거부했습니다."));
        return;
    }
    if (m_creating) {
        const auto session = results.value(QStringLiteral("session_handle")).toString();
        if (!session.startsWith(desktop + QStringLiteral("/session/"))) {
            fail(QStringLiteral("시스템에서 유효한 단축키 세션을 반환하지 않았습니다.")); return;
        }
        m_session = session;
        request(false);
    } else applyBindings(qdbus_cast<PortalShortcutList>(results.value(QStringLiteral("shortcuts"))));
}

void PortalGlobalShortcuts::applyBindings(const PortalShortcutList &shortcuts) {
    for (auto action : shortcutActions) {
        const auto found = std::find_if(shortcuts.begin(), shortcuts.end(), [action](const PortalShortcut &value) {
            return value.id == ids[int(action)];
        });
        const bool registered = found != shortcuts.end();
        emit registrationChanged(action, registered, registered ? QStringLiteral("시스템에서 지정한 전역 단축키")
            : QStringLiteral("시스템에서 이 단축키가 승인되지 않았습니다."),
            registered ? found->properties.value(QStringLiteral("trigger_description")).toString() : QString());
    }
}

void PortalGlobalShortcuts::key(const QDBusObjectPath &session, const QString &id, bool pressed, const QString &activationToken) {
    const auto index = ids.indexOf(id);
    if (m_started && session.path() == m_session && index >= 0) emit keyStateChanged(shortcutActions[index], pressed, activationToken);
}
void PortalGlobalShortcuts::activated(const QDBusObjectPath &session, const QString &id, qulonglong, const QVariantMap &options) {
    key(session, id, true, options.value(QStringLiteral("activation_token")).toString());
}
void PortalGlobalShortcuts::deactivated(const QDBusObjectPath &session, const QString &id, qulonglong, const QVariantMap &) { key(session, id, false); }
void PortalGlobalShortcuts::changed(const QDBusObjectPath &session, const PortalShortcutList &shortcuts) {
    if (m_started && session.path() == m_session) applyBindings(shortcuts);
}
void PortalGlobalShortcuts::closed(const QVariantMap &, const QDBusMessage &message) {
    if (m_started && message.path() == m_session) fail(QStringLiteral("시스템 단축키 세션이 종료되었습니다. 다시 등록하세요."));
}
