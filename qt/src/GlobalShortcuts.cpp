#include "GlobalShortcuts.h"
#include <QShortcut>
#include <QTimer>
#include <QWidget>
#include <algorithm>

GlobalShortcuts::GlobalShortcuts(QWidget *window, std::unique_ptr<ShortcutBackend> backend)
    : QObject(window), m_backend(backend ? std::move(backend) : createShortcutBackend()) {
    for (auto action : shortcutActions) {
        const auto i = int(action);
        m_local[i] = new QShortcut(shortcutKey(action), window);
        m_local[i]->setObjectName(QStringLiteral("captureShortcut%1").arg(i));
        m_local[i]->setAutoRepeat(false);
        connect(m_local[i], &QShortcut::activated, this, [this, action, i] {
            if (!m_registered[i]) emit activated(action);
        });
    }
    connect(m_backend.get(), &ShortcutBackend::registrationChanged, this,
            [this](ShortcutAction action, bool registered, const QString &detail, const QString &trigger) {
        const auto i = int(action);
        m_registered[i] = registered;
        m_held[i] = false;
        m_details[i] = detail;
        m_triggers[i] = registered && !trigger.isEmpty() ? trigger : shortcutKey(action).toString(QKeySequence::NativeText);
        m_local[i]->setEnabled(!registered);
        emit statusChanged();
    });
    connect(m_backend.get(), &ShortcutBackend::keyStateChanged, this, [this](ShortcutAction action, bool pressed, const QString &activationToken) {
        const auto i = int(action);
        if (!m_registered[i]) return;
        const bool activate = pressed && !m_held[i];
        m_held[i] = pressed;
        if (!activate) return;
        const auto generation = m_generation;
        // Never enter a modal save dialog inside a platform event callback.
        QTimer::singleShot(0, this, [this, action, i, generation, activationToken] {
            if (generation == m_generation && m_registered[i]) emit activated(action, activationToken);
        });
    });
}

GlobalShortcuts::~GlobalShortcuts() { m_backend->stop(); }

void GlobalShortcuts::retry() {
    ++m_generation;
    m_backend->stop();
    m_registered.fill(false);
    m_held.fill(false);
    for (auto action : shortcutActions) {
        m_local[int(action)]->setEnabled(true);
        m_details[int(action)] = QStringLiteral("등록 확인 중");
        m_triggers[int(action)] = shortcutKey(action).toString(QKeySequence::NativeText);
    }
    emit statusChanged();
    m_backend->start();
}

int GlobalShortcuts::registeredCount() const {
    return int(std::count(m_registered.begin(), m_registered.end(), true));
}

QString GlobalShortcuts::description() const {
    QStringList lines;
    for (auto action : shortcutActions) {
        const auto i = int(action);
        lines.append(QStringLiteral("%1 · %2\n%3: %4")
            .arg(m_triggers[i], shortcutName(action),
                 m_registered[i] ? QStringLiteral("전역 사용") : QStringLiteral("앱 내 사용"), m_details[i]));
    }
    return lines.join(QStringLiteral("\n\n"));
}
