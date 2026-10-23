#include "MainWindow.h"
#include "RegionPicker.h"
#include <QAccessible>
#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMouseEvent>
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
    static void checkMultiScreenRegions(MainWindow &window) {
        check(QGuiApplication::screens().size() >= 2, "region integration test needs multiple monitors");
        window.m_sourceType->setCurrentIndex(2);
        for (auto *screen : QGuiApplication::screens()) {
            std::cout << "Selecting on: " << screen->name().toStdString()
                      << " scale=" << screen->devicePixelRatio() << '\n';
            window.m_target->setCurrentIndex(0);
            if (screen != QGuiApplication::primaryScreen()) {
                // A newly connected display exists in QGuiApplication before
                // an old target list has been refreshed by the user.
                for (qsizetype i = 0; i < window.m_targets.size(); ++i) {
                    if (window.m_targets[i].screen != screen) continue;
                    window.m_targets.removeAt(i);
                    window.m_target->removeItem(int(i));
                    break;
                }
            }
            window.pickRegion();
            QApplication::processEvents();
            QList<QPointer<RegionPicker>> overlays;
            RegionPicker *selectedOverlay = nullptr;
            for (auto *widget : QApplication::topLevelWidgets()) {
                auto *overlay = qobject_cast<RegionPicker *>(widget);
                if (!overlay || !overlay->isVisible()) continue;
                overlays.append(overlay);
                if (overlay->screen() == screen) selectedOverlay = overlay;
            }
            check(overlays.size() == QGuiApplication::screens().size(),
                  "region selection is available on every monitor without preselecting it");
            check(selectedOverlay, "extended monitor has an interactive selection overlay");
            for (auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
                const QPointF point = type == QEvent::MouseButtonPress ? QPointF(20, 30) : QPointF(180, 140);
                QMouseEvent event(type, point, selectedOverlay->mapToGlobal(point.toPoint()),
                    Qt::LeftButton, type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(selectedOverlay, &event);
            }
            const auto selected = window.m_targets.value(window.m_target->currentIndex());
            check(selected.screen == screen && selected.region == QRect(20, 30, 160, 110)
                      && selected.regionScreenSize == screen->size() && selected.isAvailable(),
                  "dragged monitor and local crop become the recording target");
            for (const auto &overlay : overlays)
                check(!overlay->isVisible(), "all monitor overlays hide before capture can start");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            for (const auto &overlay : overlays)
                check(!overlay, "all monitor overlays are destroyed after selection");

            window.pickRegion();
            QApplication::processEvents();
            RegionPicker *cancelOverlay = nullptr;
            for (auto *widget : QApplication::topLevelWidgets())
                if (auto *overlay = qobject_cast<RegionPicker *>(widget); overlay && overlay->isVisible())
                    cancelOverlay = overlay;
            check(cancelOverlay, "selection can be reopened");
            cancelOverlay->close();
            check(window.m_targets.value(window.m_target->currentIndex()).screen == screen
                      && window.m_targets.value(window.m_target->currentIndex()).region == selected.region,
                  "cancelling on any monitor preserves the previous capture region");
            for (auto *widget : QApplication::topLevelWidgets())
                if (auto *overlay = qobject_cast<RegionPicker *>(widget))
                    check(!overlay->isVisible(), "cancellation hides every monitor overlay");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }
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
    if (app.arguments().contains(QStringLiteral("--multi-screen"))) {
        MainWindowTestPeer::checkMultiScreenRegions(window);
        std::cout << "PASS: select and cancel regions on any monitor with mixed display scales\n";
        return 0;
    }
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
