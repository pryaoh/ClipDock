#include "MainWindow.h"
#include "RegionPicker.h"
#include "GlobalShortcuts.h"
#include <QAccessible>
#include <QAction>
#include <QDir>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>
#include <QSpinBox>
#include <QOperatingSystemVersion>
#include <QFileDialog>
#include <QDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QVideoWidget>
#include <QImage>
#include <cstdlib>
#include <iostream>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class RecorderControllerTestPeer {
public:
    static QScreen *screen(const RecorderController &controller) { return controller.m_target.screen; }
    static void prepare(RecorderController &controller, const RecorderController::Target &target, bool recording = false) {
        controller.m_target = target;
        controller.m_outputPath = recording ? QStringLiteral("synthetic.mp4") : QString();
    }
};

class MainWindowTestPeer {
public:
    static void setState(MainWindow &window, RecorderController::State state) { window.updateState(state); }
    static void checkCapturePreferences(MainWindow &window) {
        auto *exclude = window.findChild<QCheckBox *>(QStringLiteral("excludeSelfCheckBox"));
        auto *minimize = window.findChild<QCheckBox *>(QStringLiteral("minimizeRecordingCheckBox"));
        check(exclude && minimize, "capture exclusion and minimization are separate user options");
        check(exclude->isChecked() && !minimize->isChecked(), "exclusion defaults on, optional minimization defaults off");
        exclude->setChecked(false);
        minimize->setChecked(true);
        {
            MainWindow restored;
            check(!restored.findChild<QCheckBox *>(QStringLiteral("excludeSelfCheckBox"))->isChecked()
                  && restored.findChild<QCheckBox *>(QStringLiteral("minimizeRecordingCheckBox"))->isChecked(),
                  "both capture preferences survive a new window instance");
        }
        exclude->setChecked(true);
        minimize->setChecked(false);
    }
    static void checkRecordingPreferences() {
        auto *screen = QGuiApplication::screens().last();
        check(!screen->name().isEmpty(), "preferences fixture provides named displays");
        const QRect region(20, 30, 160, 110);
        {
            MainWindow original;
            original.m_resolution->setCurrentIndex(original.m_resolution->findData(2560));
            original.m_fps->setCurrentIndex(original.m_fps->findData(60));
            original.m_bitrate->setValue(12345);
            original.m_sourceType->setCurrentIndex(2);
            original.setRegion(screen, region);
            original.m_sourceType->setCurrentIndex(0);
            original.m_sourceType->setCurrentIndex(2);
            check(original.m_targets.value(original.m_target->currentIndex()).region == region,
                  "last region survives switching away and back");
        }
        {
            MainWindow restored;
            check(restored.m_resolution->currentData().toInt() == 2560,
                  "output size survives window recreation");
            check(restored.m_fps->currentData().toInt() == 60 && restored.m_bitrate->value() == 12345,
                  "FPS and bitrate survive window recreation");
            const auto target = restored.m_targets.value(restored.m_target->currentIndex());
            check(restored.m_sourceType->currentIndex() == 2 && target.screen == screen
                  && target.region == region && target.isAvailable(), "last monitor and local region survive recreation");
            restored.refreshTargets();
            restored.refreshAudio();
            check(restored.m_targets.value(restored.m_target->currentIndex()).region == region,
                  "target refresh does not overwrite region through intermediate signals");
            restored.updateState(RecorderController::State::Recording);
            check(!restored.m_resetPreferences->isEnabled(), "reset unavailable during recording");
            restored.resetPreferences();
            check(restored.m_fps->currentData().toInt() == 60, "direct reset also rejects active recording");
            restored.updateState(RecorderController::State::Idle);
            restored.m_excludeSelf->setChecked(false);
            restored.m_minimizeRecording->setChecked(true);
            restored.resetPreferences();
            check(restored.m_fps->currentData().toInt() == 25 && restored.m_bitrate->value() == 5000
                  && restored.m_resolution->currentData().toInt() == 1920 && restored.m_audio->currentIndex() == 0,
                  "reset immediately restores defaults");
            check(restored.m_excludeSelf->isChecked() && !restored.m_minimizeRecording->isChecked(),
                  "reset restores capture policy defaults");
            restored.m_sourceType->setCurrentIndex(2);
            check(restored.m_targets.value(restored.m_target->currentIndex()).region.isEmpty()
                  && !restored.m_record->isEnabled(), "reset does not resurrect previous region");
            restored.resetPreferences();
        }
        {
            MainWindow restored;
            check(restored.m_fps->currentData().toInt() == 25 && !restored.m_preferences.region.present,
                  "reset remains effective after recreation");
            restored.m_sourceType->setCurrentIndex(2);
            restored.setRegion(screen, region);
        }
        QSettings().setValue(QStringLiteral("lastRegion/devicePixelRatio"), screen->devicePixelRatio() + 0.5);
        {
            MainWindow changed;
            check(changed.m_sourceType->currentIndex() == 2 && !changed.m_record->isEnabled()
                  && changed.m_regionLabel->text().contains(QStringLiteral("다시 선택")),
                  "scale changes require reselection instead of recording a different region");
            changed.resetPreferences();
        }
        QTemporaryDir folder;
        check(folder.isValid(), "save folder fixture");
        {
            MainWindow window;
            bool visited = false;
            QTimer timer;
            timer.setInterval(10);
            QObject::connect(&timer, &QTimer::timeout, &window, [&] {
                for (auto *widget : QApplication::topLevelWidgets()) {
                    auto *dialog = qobject_cast<QFileDialog *>(widget);
                    if (!dialog || !dialog->isVisible()) continue;
                    visited = true; timer.stop();
                    dialog->selectFile(folder.filePath(QStringLiteral("saved.mp4")));
                    QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
                }
            });
            QObject::connect(&window.m_controller, &RecorderController::stateChanged, &window, [&](auto state) {
                if (state == RecorderController::State::WaitingForPermission) window.m_controller.stop();
            });
            timer.start();
            window.begin(true);
            check(visited && window.m_preferences.outputFolder == folder.path(), "accepting output remembers parent folder");
        }
        {
            MainWindow restored;
            bool visited = false;
            QTimer timer;
            timer.setInterval(10);
            QObject::connect(&timer, &QTimer::timeout, &restored, [&] {
                for (auto *widget : QApplication::topLevelWidgets()) {
                    auto *dialog = qobject_cast<QFileDialog *>(widget);
                    if (!dialog || !dialog->isVisible()) continue;
                    check(dialog->directory().absolutePath() == folder.path(), "save dialog opens remembered folder");
                    visited = true; timer.stop();
                    dialog->setDirectory(QDir::tempPath());
                    dialog->reject();
                }
            });
            timer.start();
            restored.begin(true);
            check(visited && restored.m_preferences.outputFolder == folder.path(), "cancel does not change remembered folder");
            check(restored.m_controller.state() == RecorderController::State::Idle && QDir(folder.path()).isEmpty(),
                  "cancelled or stopped startup creates no output");
            restored.resetPreferences();
        }
        {
            MainWindow stale;
            stale.m_sourceType->setCurrentIndex(2);
            stale.setRegion(screen, region);
            // Simulate a selection made in a different coordinate scale. The UI target
            // still has the same screen pointer and logical size, as during a modal dialog.
            stale.m_preferences.region.screen.devicePixelRatio += 0.5;
            bool started = false, visited = false;
            QObject::connect(&stale.m_controller, &RecorderController::stateChanged, &stale, [&](auto state) {
                if (state == RecorderController::State::WaitingForPermission) {
                    started = true; stale.m_controller.stop();
                }
            });
            QTimer timer;
            timer.setInterval(10);
            QObject::connect(&timer, &QTimer::timeout, &stale, [&] {
                for (auto *widget : QApplication::topLevelWidgets()) {
                    if (auto *message = qobject_cast<QMessageBox *>(widget); message && message->isVisible())
                        message->accept();
                    if (auto *dialog = qobject_cast<QFileDialog *>(widget); dialog && dialog->isVisible()) {
                        visited = true;
                        dialog->selectFile(folder.filePath(QStringLiteral("stale.mp4")));
                        QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
                    }
                }
            });
            timer.start();
            stale.begin(true);
            check(visited && !started, "stale region scale is rejected after accepting the save dialog");
        }
        {
            QTemporaryDir disappearingFolder;
            check(disappearingFolder.isValid(), "removable folder fixture");
            QSettings().setValue(QStringLiteral("recording/outputFolder"), disappearingFolder.path());
            QSettings().setValue(QStringLiteral("recording/sourceType"), QStringLiteral("screen"));
            MainWindow window;
            check(QDir().rmdir(disappearingFolder.path()), "delete empty fixture after preferences load");
            bool visited = false;
            QTimer timer;
            timer.setInterval(10);
            QObject::connect(&timer, &QTimer::timeout, &window, [&] {
                for (auto *widget : QApplication::topLevelWidgets()) {
                    auto *dialog = qobject_cast<QFileDialog *>(widget);
                    if (!dialog || !dialog->isVisible()) continue;
                    check(dialog->directory().exists() && dialog->directory().absolutePath() != disappearingFolder.path(),
                          "folder deleted after load uses a reachable save location");
                    visited = true; timer.stop(); dialog->reject();
                }
            });
            timer.start(); window.begin(true);
            check(visited, "missing folder still opens save dialog");
        }
        QSettings().clear();
        std::cout << "PASS: UI preferences, region restore, invalid scale, reset and save folder\n";
    }
    static void begin(MainWindow &window) { window.begin(true); }
    static void clearTargets(MainWindow &window) {
        window.m_targets.clear();
        window.m_target->clear();
    }
    static RecorderController &controller(MainWindow &window) { return window.m_controller; }
    static void checkStartupVisible(MainWindow &window) {
        RecorderController::Target target;
        target.screen = QGuiApplication::primaryScreen();
        RecorderControllerTestPeer::prepare(window.m_controller, target, true);
        window.findChild<QCheckBox *>(QStringLiteral("minimizeRecordingCheckBox"))->setChecked(true);
        window.show();
        window.updateState(RecorderController::State::Starting);
        check(!window.isMinimized(), "screen permission and portal selection keep the main controls visible");
        window.m_controller.recordingAboutToStart();
        check(window.isMinimized(), "the prepared writer boundary triggers requested minimization");
    }
    static void checkMoveIntoCapture(MainWindow &window) {
        RecorderController::Target target;
        target.screen = QGuiApplication::primaryScreen();
        RecorderControllerTestPeer::prepare(window.m_controller, target, true);
        window.move(target.screen->geometry().topRight() + QPoint(100, 0));
        window.show();
        window.updateState(RecorderController::State::Recording);
        int pauses = 0;
        const auto connection = QObject::connect(window.m_pause, &QPushButton::clicked, [&] { ++pauses; });
        window.move(target.screen->geometry().topLeft() + QPoint(10, 10));
        QApplication::processEvents();
        check(pauses == 1, "moving a visible recorder into capture pauses unsupported exclusion");
        QObject::disconnect(connection);
    }
    static void checkCapturePolicy(MainWindow &window) {
        using State = RecorderController::State;
        auto *exclude = window.findChild<QCheckBox *>(QStringLiteral("excludeSelfCheckBox"));
        auto *minimize = window.findChild<QCheckBox *>(QStringLiteral("minimizeRecordingCheckBox"));
        RecorderController::Target target;
        target.screen = QGuiApplication::primaryScreen();
        RecorderControllerTestPeer::prepare(window.m_controller, target);
        window.move(target.screen->geometry().topLeft() + QPoint(10, 10));
        window.show();
        window.updateState(State::Preview);
        check(!window.isMinimized() && !window.m_video->isVisible(),
              "unsupported overlapping preview shows guidance without minimizing");
        window.move(target.screen->geometry().topRight() + QPoint(100, 0));
        QApplication::processEvents();
        check(window.m_video->isVisible(), "moving outside the captured monitor restores live preview");
        window.move(target.screen->geometry().topLeft() + QPoint(10, 10));
        QApplication::processEvents();
        check(!window.m_video->isVisible(), "moving back into capture blocks recursive preview");
        target.isRegion = true;
        target.region = QRect(0, 0, 40, 40);
        target.regionScreenSize = target.screen->size();
        RecorderControllerTestPeer::prepare(window.m_controller, target);
        window.move(target.screen->geometry().topLeft() + QPoint(100, 100));
        QApplication::processEvents();
        check(window.m_video->isVisible(), "non-overlapping region on the same monitor keeps preview");
        target.isRegion = false;
        target.isWindow = true;
        RecorderControllerTestPeer::prepare(window.m_controller, target);
        window.updateState(State::Preview);
        check(window.m_video->isVisible(), "capture of another window keeps live preview");
        target.isWindow = false;
        target.isPortal = true;
        RecorderControllerTestPeer::prepare(window.m_controller, target);
        window.updateState(State::Preview);
        check(!window.m_video->isVisible(), "unknown portal source conservatively suppresses recursive preview");
        target.isPortal = false;
        RecorderControllerTestPeer::prepare(window.m_controller, target, true);
        window.updateState(State::WaitingForPermission);
        check(!window.isMinimized(), "permission UI remains reachable");
        window.updateState(State::Starting);
        check(!window.isMinimized(), "Starting may still wait for screen permission and must remain visible");
        window.m_controller.recordingAboutToStart();
        check(window.isMinimized(), "unsupported exclusion minimizes overlapping recorder before capture");
        window.updateState(State::Recording);
        int pauseRequests = 0;
        const auto pauseConnection = QObject::connect(window.m_pause, &QPushButton::clicked, [&] { ++pauseRequests; });
        window.showNormal();
        QApplication::processEvents();
        check(pauseRequests == 1 && window.m_stop->isEnabled(),
              "restoring unsupported overlapping recorder pauses through the normal control and keeps stop reachable");
        // The synthetic controller has no encoder to acknowledge the pause.
        window.updateState(State::Paused);
        window.m_pause->click();
        check(window.isMinimized(), "resuming fallback capture minimizes before sending the resume command");
        QObject::disconnect(pauseConnection);
        window.updateState(State::Idle);
        check(!window.isMinimized(), "startup failure restores an automatically minimized window");
        exclude->setChecked(false);
        window.updateState(State::Starting);
        window.m_controller.recordingAboutToStart();
        check(!window.isMinimized(), "explicit opt-out does not force minimization");
        window.updateState(State::Idle);
        minimize->setChecked(true);
        RecorderControllerTestPeer::prepare(window.m_controller, target, false);
        window.updateState(State::Starting);
        window.updateState(State::Preview);
        check(!window.isMinimized(), "optional minimization never hides preview-only sessions");
        window.updateState(State::Idle);
        RecorderControllerTestPeer::prepare(window.m_controller, target, true);
        window.showMaximized();
        window.updateState(State::Starting);
        window.m_controller.recordingAboutToStart();
        check(window.isMinimized(), "explicit minimization also works with exclusion disabled");
        window.updateState(State::Recording);
        window.updateState(State::Stopping);
        window.updateState(State::Idle);
        check(window.isMaximized(), "completion restores the pre-recording maximized state");
        window.showNormal();
        window.showMinimized();
        window.updateState(State::Starting);
        window.m_controller.recordingAboutToStart();
        window.updateState(State::Idle);
        check(window.isMinimized(), "user-minimized windows are not restored by the app");
        window.showNormal();
        window.updateState(State::Starting);
        window.m_controller.recordingAboutToStart();
        window.m_closing = true;
        window.updateState(State::Idle);
        check(window.isMinimized(), "closing after finalization does not reopen an automatically minimized window");
        window.m_closing = false;
        window.showNormal();
        exclude->setChecked(true);
        minimize->setChecked(false);
        RecorderControllerTestPeer::prepare(window.m_controller, {});
        window.hide();
    }
    static void checkShortcutGuards(MainWindow &window) {
        using State = RecorderController::State;
        int pauses = 0, stops = 0;
        const auto pauseConnection = QObject::connect(window.m_pause, &QPushButton::clicked, [&] { ++pauses; });
        const auto stopConnection = QObject::connect(window.m_stop, &QPushButton::clicked, [&] { ++stops; });
        window.updateState(State::Idle);
        window.dispatchShortcut(ShortcutAction::Pause);
        window.dispatchShortcut(ShortcutAction::Stop);
        check(pauses == 0 && stops == 0, "idle shortcuts cannot pause or stop capture");
        window.updateState(State::Recording);
        window.dispatchShortcut(ShortcutAction::Pause);
        check(pauses == 1, "pause shortcut follows the enabled recording control");
        QDialog dialog(&window);
        dialog.setModal(true); dialog.show(); QApplication::processEvents();
        const auto previousToken = qgetenv("XDG_ACTIVATION_TOKEN");
        window.dispatchShortcut(ShortcutAction::Record, QStringLiteral("ignored-token"));
        check(qgetenv("XDG_ACTIVATION_TOKEN") == previousToken, "ignored shortcut does not publish an activation token");
        window.dispatchShortcut(ShortcutAction::Pause);
        window.dispatchShortcut(ShortcutAction::Stop);
        check(pauses == 1 && stops == 0, "modal dialogs block global recording commands");
        dialog.hide();
        window.m_selectingOutput = true;
        window.dispatchShortcut(ShortcutAction::Stop);
        check(stops == 0, "pending save request blocks shortcut reentry");
        window.m_selectingOutput = false;
        window.updateState(State::Stopping);
        window.dispatchShortcut(ShortcutAction::Pause);
        window.dispatchShortcut(ShortcutAction::Stop);
        check(pauses == 1 && stops == 0, "finalization rejects repeated capture commands");
        QObject::disconnect(pauseConnection); QObject::disconnect(stopConnection);
        window.updateState(State::Idle);
    }
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
        RecorderController::Target target;
        target.isWindow = true;
        RecorderControllerTestPeer::prepare(window.m_controller, target);
        int captureShortcuts = 0;
        for (auto *shortcut : window.findChildren<QShortcut *>()) {
            if (shortcut->key() != QKeySequence(QStringLiteral("Ctrl+Shift+R"))
                && shortcut->key() != QKeySequence(QStringLiteral("Ctrl+Shift+P"))
                && shortcut->key() != QKeySequence(QStringLiteral("Ctrl+Shift+S"))) continue;
            ++captureShortcuts;
            check(!shortcut->autoRepeat(), "holding a recording shortcut must not repeatedly dispatch commands");
        }
        check(captureShortcuts == 3, "record, pause and stop retain app-local fallback shortcuts");
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

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    app.setStyle(QStringLiteral("Fusion"));
    app.setQuitOnLastWindowClosed(false);
    QTemporaryDir directory;
    check(directory.isValid(), "temporary directory");
    QCoreApplication::setOrganizationName(QStringLiteral("ClipDockTests"));
    QCoreApplication::setApplicationName(QStringLiteral("CapturePolicy"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    if (app.arguments().contains(QStringLiteral("--preferences"))) {
        MainWindowTestPeer::checkRecordingPreferences();
        return 0;
    }
    MainWindow window;
    if (app.arguments().contains(QStringLiteral("--startup-policy"))) {
        MainWindowTestPeer::checkStartupVisible(window);
        return 0;
    }
    if (app.arguments().contains(QStringLiteral("--move-into-capture"))) {
        MainWindowTestPeer::checkMoveIntoCapture(window);
        return 0;
    }
#ifdef Q_OS_WIN
    if (app.arguments().contains(QStringLiteral("--capture-exclusion"))) {
        // Opt-in desktop test: prove pixels behind ClipDock are captured, not a
        // black rectangle or the recorder itself. No desktop images are saved.
        QWidget background;
        background.setStyleSheet(QStringLiteral("background: #00ff00;"));
        background.setGeometry(app.primaryScreen()->geometry());
        background.show();
        window.move(app.primaryScreen()->availableGeometry().topLeft() + QPoint(40, 40));
        window.show();
        QLabel marker(&window);
        marker.setStyleSheet(QStringLiteral("background: #ff00ff;"));
        marker.setGeometry(100, 100, 160, 160);
        marker.show(); marker.raise(); window.raise();
        QMediaCaptureSession session;
        QScreenCapture capture;
        QVideoSink sink;
        session.setScreenCapture(&capture);
        session.setVideoSink(&sink);
        capture.setScreen(app.primaryScreen());
        bool baselineSeen = false, restored = false;
        int excludedFrames = 0;
        QObject::connect(&sink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
            const auto image = frame.toImage();
            if (image.isNull()) return;
            const auto screenRect = app.primaryScreen()->geometry();
            const auto point = marker.mapToGlobal(marker.rect().center()) - screenRect.topLeft();
            const QColor color = image.pixelColor(point.x() * image.width() / screenRect.width(),
                                                   point.y() * image.height() / screenRect.height());
            if (!baselineSeen) {
                if (color.red() < 220 || color.blue() < 220 || color.green() > 30) return;
                baselineSeen = true;
                MainWindowTestPeer::setState(window, RecorderController::State::Preview);
                return;
            }
            if (restored) {
                if (color.red() > 220 && color.blue() > 220 && color.green() < 30) app.exit(0);
                return;
            }
            if (color.green() > 220 && color.red() < 30 && color.blue() < 30) {
                if (++excludedFrames == 3) {
                    restored = true;
                    MainWindowTestPeer::setState(window, RecorderController::State::Idle);
                }
            } else excludedFrames = 0;
        });
        QObject::connect(&capture, &QScreenCapture::errorOccurred, &app, [&](auto, const QString &message) {
            std::cerr << "FAIL capture: " << message.toStdString() << '\n'; app.exit(1);
        });
        QTimer::singleShot(15000, &app, [&] {
            std::cerr << "FAIL: real screen capture exclusion; baseline=" << baselineSeen << '\n'; app.exit(1);
        });
        QTimer::singleShot(500, &app, [&] {
            capture.start();
        });
        const int result = app.exec();
        capture.stop();
        if (!result) std::cout << "PASS: real screen frames contain the background behind ClipDock\n";
        return result;
    }
    if (app.arguments().contains(QStringLiteral("--capture-affinity"))) {
        window.show();
        QApplication::processEvents();
        auto checkAffinity = [&](QWidget &widget, DWORD expected) {
            DWORD affinity = WDA_NONE;
            const bool readable = GetWindowDisplayAffinity(reinterpret_cast<HWND>(widget.winId()), &affinity);
            if (!readable || affinity != expected)
                std::cerr << "affinity " << widget.metaObject()->className() << " expected=" << expected
                          << " actual=" << affinity << " readable=" << readable << '\n';
            check(readable && affinity == expected, "native capture affinity matches preference and lifecycle");
        };
        const bool supported = QOperatingSystemVersion::current()
            >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 19041);
        const DWORD activeAffinity = supported ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        checkAffinity(window, WDA_NONE);
        RecorderController::Target captureTarget;
        captureTarget.screen = app.primaryScreen();
        window.move(captureTarget.screen->geometry().topLeft() + QPoint(40, 40));
        RecorderControllerTestPeer::prepare(MainWindowTestPeer::controller(window), captureTarget, true);
        MainWindowTestPeer::setState(window, RecorderController::State::Starting);
        MainWindowTestPeer::controller(window).recordingAboutToStart();
        check(window.isMinimized() != supported, "native supported exclusion keeps the default recording controls visible");
        MainWindowTestPeer::setState(window, RecorderController::State::Idle);
        MainWindowTestPeer::setState(window, RecorderController::State::Preview);
        checkAffinity(window, activeAffinity);
        QDialog dialog(&window);
        dialog.show();
        QApplication::processEvents();
        checkAffinity(dialog, activeAffinity);
        QWidget unrelated;
        unrelated.show();
        QApplication::processEvents();
        checkAffinity(unrelated, WDA_NONE);
        unrelated.hide();
        auto *exclude = window.findChild<QCheckBox *>(QStringLiteral("excludeSelfCheckBox"));
        exclude->setChecked(false);
        checkAffinity(window, WDA_NONE);
        checkAffinity(dialog, WDA_NONE);
        exclude->setChecked(true);
        checkAffinity(window, activeAffinity);
        dialog.hide();
        window.hide();
        // Changing flags can recreate the native window; the exclusion must survive.
        window.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        window.show();
        QApplication::processEvents();
        checkAffinity(window, activeAffinity);
        MainWindowTestPeer::setState(window, RecorderController::State::Idle);
        checkAffinity(window, WDA_NONE);
        checkAffinity(dialog, WDA_NONE);
        std::cout << "PASS: main window capture exclusion survives native window changes\n";
        return 0;
    }
#endif
    if (app.arguments().contains(QStringLiteral("--multi-screen"))) {
        MainWindowTestPeer::checkMultiScreenRegions(window);
        std::cout << "PASS: select and cancel regions on any monitor with mixed display scales\n";
        return 0;
    }
    MainWindowTestPeer::checkCapturePreferences(window);
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
            MainWindowTestPeer::begin(window); // Must return while the first save dialog is open.
            // Reproduce a hotplug refresh while the save dialog runs its nested event loop.
            MainWindowTestPeer::clearTargets(window);
            dialog->selectFile(directory.filePath(QStringLiteral("snapshot.mp4")));
            QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
            break;
        }
    });
    QTimer dialogDeadline;
    dialogDeadline.setSingleShot(true);
    QObject::connect(&dialogDeadline, &QTimer::timeout, &app, [] { check(false, "save dialog timed out"); });
    dialogDeadline.start(5000);
    dialogTimer.start();
    MainWindowTestPeer::begin(window);
    dialogDeadline.stop();
    check(dialogVisited && started, "target snapshot survives target list refresh");
    check(controller.state() == RecorderController::State::Idle, "cancelled startup is idle");
    MainWindowTestPeer::checkPortalTargets(window);
    MainWindowTestPeer::checkCapturePolicy(window);
    MainWindowTestPeer::checkPresentationStates(window);
    MainWindowTestPeer::checkShortcutGuards(window);
    std::cout << "PASS: save dialog target-list refresh retains selection without stale index access\n";
}
