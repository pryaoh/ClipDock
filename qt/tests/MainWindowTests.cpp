#include "MainWindow.h"
#include <QAccessible>
#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QVideoWidget>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class MainWindowTestPeer {
public:
    static void begin(MainWindow &window) { window.begin(true); }
    static void clearTargets(MainWindow &window) {
        window.m_targets.clear();
        window.m_target->clear();
    }
    static RecorderController &controller(MainWindow &window) { return window.m_controller; }
    static void checkPresentationStates(MainWindow &window) {
        using State = RecorderController::State;
        window.show();
        QApplication::processEvents();
        auto *preview = window.findChild<QStackedWidget *>(QStringLiteral("previewStack"));
        auto *badge = window.findChild<QLabel *>(QStringLiteral("stateBadge"));
        check(preview && badge, "preview has an empty state and an accessible recording status");
        const auto *accessibleTimer = QAccessible::queryAccessibleInterface(window.m_clock);
        window.m_controller.durationChanged(62000);
        check(accessibleTimer && accessibleTimer->text(QAccessible::Name).contains(QStringLiteral("00:01:02")),
              "assistive technology can read the elapsed time, not just a static label");
        window.m_controller.durationChanged(0);
        window.updateState(State::Idle);
        check(preview->currentWidget() != window.m_video && !window.m_video->isVisible(),
              "idle hides the stale video behind the empty-state guidance");
        for (auto state : {State::WaitingForPermission, State::Starting}) {
            window.updateState(state);
            check(preview->currentWidget() != window.m_video && !window.m_record->isEnabled(),
                  "capture startup displays guidance and prevents a second start");
        }
        window.updateState(State::Preview);
        check(window.m_video->isVisible() && window.m_record->isEnabled() && window.m_stop->isEnabled(),
              "preview reveals video and allows recording or stopping");
        window.updateState(State::Recording);
        check(window.m_video->isVisible() && window.m_pause->isEnabled() && !window.m_settings->isEnabled()
              && badge->text().contains(QStringLiteral("녹화 중")),
              "recording keeps the video visible, locks settings and labels the recording state");
        window.updateState(State::Paused);
        check(window.m_video->isVisible() && window.m_pause->isEnabled()
              && window.m_pause->text() == QStringLiteral("재개")
              && badge->text().contains(QStringLiteral("일시정지")),
              "paused recording retains the picture and exposes resume with a distinct status");
        const auto *accessibleBadge = QAccessible::queryAccessibleInterface(badge);
        check(accessibleBadge && accessibleBadge->text(QAccessible::Name).contains(QStringLiteral("일시정지")),
              "assistive technology can read the current capture state");
        window.updateState(State::Stopping);
        check(!window.m_record->isEnabled() && !window.m_pause->isEnabled() && !window.m_stop->isEnabled(),
              "finalizing a file disables capture commands");
        window.updateState(State::Idle);
        check(preview->currentWidget() != window.m_video && window.m_settings->isEnabled(),
              "returning to idle restores guidance and editable settings");
        window.hide();
    }
    static void checkPortalTargets(MainWindow &window) {
        window.m_portalCapture = true;
        window.refreshTargets();
        check(window.m_targets.size() == 1 && window.m_targets.front().isPortal,
              "Wayland exposes a portal target rather than a misleading monitor list");
        check(!window.m_target->isEnabled(), "Wayland monitor selection is delegated to the portal");
        check(window.m_targets.front().isAvailable() && !window.m_targets.front().screen,
              "portal is selectable without binding it to a QScreen");
    }
};

class RecorderControllerTestPeer {
public:
    static QScreen *screen(const RecorderController &controller) { return controller.m_target.screen; }
};

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    app.setStyle(QStringLiteral("Fusion"));
    app.setQuitOnLastWindowClosed(false);
    QTemporaryDir directory;
    check(directory.isValid(), "temporary directory");
    MainWindow window;
    auto &controller = MainWindowTestPeer::controller(window);
    bool started = false, dialogVisited = false;
    QObject::connect(&controller, &RecorderController::stateChanged, &app, [&](auto state) {
        if (state != RecorderController::State::WaitingForPermission) return;
        started = true;
        check(RecorderControllerTestPeer::screen(controller) == app.primaryScreen(),
              "recording retains selected screen after target list changes");
        controller.stop(); // Exercise selection without requesting OS capture permissions.
    });
    QTimer dialogTimer;
    dialogTimer.setInterval(10);
    QObject::connect(&dialogTimer, &QTimer::timeout, &app, [&] {
        for (auto *widget : QApplication::topLevelWidgets()) {
            auto *dialog = qobject_cast<QFileDialog *>(widget);
            if (!dialog || !dialog->isVisible()) continue;
            dialogTimer.stop();
            dialogVisited = true;
            // Reproduce a hotplug refresh while the save dialog runs its nested event loop.
            MainWindowTestPeer::clearTargets(window);
            dialog->selectFile(directory.filePath(QStringLiteral("snapshot.mp4")));
            QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
            break;
        }
    });
    QTimer::singleShot(5000, &app, [] { check(false, "save dialog timed out"); });
    dialogTimer.start();
    MainWindowTestPeer::begin(window);
    check(dialogVisited && started, "target snapshot survives target list refresh");
    check(controller.state() == RecorderController::State::Idle, "cancelled startup is idle");
    MainWindowTestPeer::checkPortalTargets(window);
    MainWindowTestPeer::checkPresentationStates(window);
    std::cout << "PASS: save dialog target-list refresh retains selection without stale index access\n";
}
