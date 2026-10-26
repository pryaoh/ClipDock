#pragma once
#include "ShortcutBackend.h"
#include <array>

class QWidget;
class QShortcut;

class GlobalShortcuts final : public QObject {
    Q_OBJECT
public:
    explicit GlobalShortcuts(QWidget *window, std::unique_ptr<ShortcutBackend> backend = {});
    ~GlobalShortcuts() override;
    void retry();
    int registeredCount() const;
    QString description() const;
signals:
    void activated(ShortcutAction action, const QString &activationToken = QString());
    void statusChanged();
private:
    std::unique_ptr<ShortcutBackend> m_backend;
    std::array<QShortcut *, 3> m_local{};
    std::array<bool, 3> m_registered{}, m_held{};
    std::array<QString, 3> m_details{};
    std::array<QString, 3> m_triggers{};
    unsigned m_generation = 0;
};
