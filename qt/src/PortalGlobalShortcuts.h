#pragma once
#include "ShortcutBackend.h"
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QVariantMap>

struct PortalShortcut { QString id; QVariantMap properties; };
using PortalShortcutList = QList<PortalShortcut>;
Q_DECLARE_METATYPE(PortalShortcut)
Q_DECLARE_METATYPE(PortalShortcutList)
QDBusArgument &operator<<(QDBusArgument &argument, const PortalShortcut &shortcut);
const QDBusArgument &operator>>(const QDBusArgument &argument, PortalShortcut &shortcut);

class PortalGlobalShortcuts final : public ShortcutBackend {
    Q_OBJECT
public:
    explicit PortalGlobalShortcuts(const QString &service = QStringLiteral("org.freedesktop.portal.Desktop"));
    ~PortalGlobalShortcuts() override;
    void start() override;
    void stop() override;
private slots:
    void response(uint code, const QVariantMap &results, const QDBusMessage &message);
    void activated(const QDBusObjectPath &session, const QString &id, qulonglong, const QVariantMap &);
    void deactivated(const QDBusObjectPath &session, const QString &id, qulonglong, const QVariantMap &);
    void changed(const QDBusObjectPath &session, const PortalShortcutList &shortcuts);
    void closed(const QVariantMap &, const QDBusMessage &message);
private:
    void checkCapability(unsigned generation);
    void request(bool create);
    void applyBindings(const PortalShortcutList &shortcuts);
    void fail(const QString &message);
    void key(const QDBusObjectPath &session, const QString &id, bool pressed, const QString &activationToken = QString());
    QString m_connectionName;
    QDBusConnection m_bus;
    QString m_service, m_request, m_session;
    bool m_creating = false, m_started = false;
    unsigned m_generation = 0;
};
