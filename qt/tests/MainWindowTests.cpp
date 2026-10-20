#include "MainWindow.h"
#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QScreen>
#include <QTemporaryDir>
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
};

class RecorderControllerTestPeer {
public:
    static QScreen *screen(const RecorderController &controller) { return controller.m_target.screen; }
};

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
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
    std::cout << "PASS: save dialog target-list refresh retains selection without stale index access\n";
}
