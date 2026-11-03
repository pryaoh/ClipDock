#pragma once
#include <QObject>
#include <QKeySequence>
#include <array>
#include <memory>

enum class ShortcutAction { Record, Pause, Stop };
Q_DECLARE_METATYPE(ShortcutAction)
inline constexpr std::array shortcutActions {ShortcutAction::Record, ShortcutAction::Pause, ShortcutAction::Stop};
inline QKeySequence shortcutKey(ShortcutAction action) {
    const int keys[] = {Qt::Key_R, Qt::Key_P, Qt::Key_S};
    return QKeySequence(Qt::CTRL | Qt::SHIFT | keys[int(action)]);
}
inline QString shortcutName(ShortcutAction action) {
    switch (action) {
    case ShortcutAction::Record: return QStringLiteral("녹화 시작");
    case ShortcutAction::Pause: return QStringLiteral("일시정지 / 재개");
    case ShortcutAction::Stop: return QStringLiteral("중지 및 저장");
    }
    return {};
}

class ShortcutBackend : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void start() = 0;
    virtual void stop() = 0;
signals:
    void registrationChanged(ShortcutAction action, bool registered, const QString &detail,
                             const QString &trigger = QString());
    void keyStateChanged(ShortcutAction action, bool pressed, const QString &activationToken = QString());
};

std::unique_ptr<ShortcutBackend> createShortcutBackend();
