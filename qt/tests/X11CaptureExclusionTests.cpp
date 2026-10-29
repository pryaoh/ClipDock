#include "MainWindow.h"
#include "GlobalShortcuts.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QColor>
#include <QUrl>
#include <QLabel>
#include <QMediaPlayer>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QVideoWidget>
#include <X11/Xlib.h>
#include <iostream>

class MainWindowTestPeer {
public:
    static RecorderController &controller(MainWindow &w) { return w.m_controller; }
    static QVideoSink *sink(MainWindow &w) { return w.m_video->videoSink(); }
    static void resume(MainWindow &w) { w.m_pause->click(); }
    static bool canStop(MainWindow &w) { return w.m_stop->isEnabled(); }
    static int shortcuts(MainWindow &w) { return w.m_shortcuts->registeredCount(); }
    static bool previewHidden(MainWindow &w) { return w.m_previewStack->currentIndex() == 0 && !w.m_video->isVisible(); }
    static void disableExclusion(MainWindow &w) { w.m_excludeSelf->setChecked(false); }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    std::cout << std::unitbuf;
    app.setQuitOnLastWindowClosed(false);
    app.setStyle(QStringLiteral("Fusion"));
    QCoreApplication::setOrganizationName("ClipDockDesktopTest");
    QCoreApplication::setApplicationName("CapturePolicy");
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QTemporaryDir recordingsDirectory;
    if (!settings.isValid() || !recordingsDirectory.isValid()) return 2;
    const QString output = recordingsDirectory.filePath("recording.mp4");
    const bool direct = app.arguments().contains("--direct");
    const bool region = app.arguments().contains("--region");
    const bool optOut = app.arguments().contains("--opt-out");
    QWidget background;
    background.setStyleSheet("background: #00ff00");
    background.setGeometry(app.primaryScreen()->geometry());
    background.show();
    MainWindow window;
    window.move(20, 20);
    window.show();
    if (optOut) MainWindowTestPeer::disableExclusion(window);
    QLabel marker(&window);
    marker.setStyleSheet("background: #ff00ff");
    marker.setGeometry(300, 180, 200, 160);
    marker.show(); marker.raise(); window.raise();
    auto &controller = MainWindowTestPeer::controller(window);
    RecorderController::Target target;
    target.screen = app.primaryScreen();
    if (region) {
        target.isRegion = true;
        target.regionScreenSize = target.screen->size();
        target.region = QRect(100, 100, 900, 600);
    }
    RecordingOptions options;
    options.fps = 10; options.maxEdge = 1280; options.microphone = false;
    QMediaPlayer player;
    QProcess otherApp;
    otherApp.start("xmessage", {"-title", "ClipDockTestFocus", "-geometry", "100x50+1100+700", "-buttons", "OK:0", "Focus test"});
    if (!otherApp.waitForStarted(3000)) return 2;
    QVideoSink decodedSink;
    player.setVideoSink(&decodedSink);
    int recordings = 0, pauses = 0, decoded = 0, green = 0, earlyMagenta = 0, allMagenta = 0;
    bool baseline = false, saved = false, finished = false;
    auto fail = [&](const QString &message) {
        std::cerr << "FAIL: " << message.toStdString() << '\n'; app.exit(1);
    };
    auto visibleInX11 = [&] {
        Display *display = XOpenDisplay(nullptr);
        XWindowAttributes attributes{};
        bool visible = display && XGetWindowAttributes(display, window.winId(), &attributes)
            && attributes.map_state == IsViewable;
        if (display) XCloseDisplay(display);
        return visible;
    };
    auto pixel = [&](const QVideoFrame &frame) {
        const QImage image = frame.toImage();
        const auto bounds = region ? target.region.translated(target.screen->geometry().topLeft())
                                   : target.screen->geometry();
        const QPoint point = window.mapToGlobal(QPoint(400, 260)) - bounds.topLeft();
        return image.isNull() ? QColor() : image.pixelColor(point.x() * image.width() / bounds.width(),
                                                           point.y() * image.height() / bounds.height());
    };
    auto key = [&](const QString &sequence) {
        for (const QStringList args : {QStringList{"search", "--name", "^ClipDockTestFocus$", "windowactivate", "--sync"},
                                      QStringList{"key", "--clearmodifiers", sequence}}) {
            QProcess command;
            command.start("xdotool", args);
            if (!command.waitForFinished(10000) || command.exitCode() != 0) {
                std::cerr << "xdotool " << args.join(' ').toStdString() << " exit=" << command.exitCode()
                          << " error=" << command.errorString().toStdString() << " stderr=" << command.readAllStandardError().toStdString()
                          << " other_state=" << int(otherApp.state()) << " other_exit=" << otherApp.exitCode()
                          << " other_stderr=" << otherApp.readAllStandardError().toStdString() << '\n';
                command.kill(); command.waitForFinished();
                fail("external-app focus or XTest key injection failed"); return;
            }
        }
    };
    QObject::connect(&controller, &RecorderController::errorOccurred, &app, fail);
    QObject::connect(MainWindowTestPeer::sink(window), &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid() || baseline || controller.state() != RecorderController::State::Preview) return;
        const auto color = pixel(frame);
        if (color.red() < 200 || color.blue() < 200 || color.green() > 40) return;
        if (!MainWindowTestPeer::previewHidden(window)) { fail("overlapping preview did not hide recursive video"); return; }
        baseline = true;
        std::cout << "PASS: real X11 capture sees the recorder while recursive UI preview is hidden\n";
        QTimer::singleShot(0, &app, [&] {
            controller.start(target, options, {}, QMediaFormat::VideoCodec::MPEG4, output);
        });
    });
    QObject::connect(&controller, &RecorderController::saved, &app, [&](const QString &path) {
        saved = path == output && QFileInfo(path).size() > 0;
    });
    QObject::connect(&controller, &RecorderController::stateChanged, &app, [&](auto state) {
        using State = RecorderController::State;
        std::cout << "state=" << int(state) << " minimized=" << window.isMinimized() << '\n';
        if (state == State::Recording) {
            ++recordings;
            if (optOut) {
                if (window.isMinimized()) { fail("opt-out unexpectedly minimized the recorder"); return; }
                QTimer::singleShot(1200, &app, [&] { controller.stop(); });
                return;
            }
            std::cout << "recording_transition=" << recordings << " x11_viewable=" << visibleInX11() << '\n';
            if (!window.isMinimized()) { fail("recording did not minimize unsupported overlapping UI"); return; }
            QTimer::singleShot(400, &app, [&] {
                if (visibleInX11()) fail("window manager still maps the minimized recorder");
            });
            if (recordings == 1) QTimer::singleShot(1000, &app, [&] { key("ctrl+shift+p"); });
            else if (recordings == 2) QTimer::singleShot(900, &app, [&] {
                QProcess command;
                command.start("xdotool", {"windowactivate", "--sync", QString::number(window.winId())});
                if (!command.waitForFinished(10000) || command.exitCode() != 0) {
                    command.kill(); command.waitForFinished();
                    fail("window manager restore failed");
                }
            });
            else if (recordings == 3) QTimer::singleShot(1000, &app, [&] { key("ctrl+shift+s"); });
            else fail("unexpected extra recording transition");
        } else if (state == State::Paused) {
            ++pauses;
            if (pauses == 1) {
                std::cout << "PASS: global key paused the real encoder with another app focused\n";
                QTimer::singleShot(300, &app, [&] { key("ctrl+shift+p"); });
            } else if (pauses == 2) {
                if (window.isMinimized() || !MainWindowTestPeer::canStop(window)) {
                    fail("restored controls are not reachable"); return;
                }
                std::cout << "PASS: restoring the recorder automatically paused the real encoder\n";
                QTimer::singleShot(300, &app, [&] { MainWindowTestPeer::resume(window); });
            } else fail("unexpected pause");
        } else if (state == State::Idle && recordings > 0 && !finished) {
            finished = true;
            if (!saved || window.isMinimized() || recordings != (optOut ? 1 : 3) || pauses != (optOut ? 0 : 2)) {
                fail("completion did not restore the recorder and preserve the movie"); return;
            }
            player.setSource(QUrl::fromLocalFile(output));
            player.play();
        }
    });
    QObject::connect(&decodedSink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid()) return;
        ++decoded;
        const auto color = pixel(frame);
        if (color.green() > 200 && color.red() < 40 && color.blue() < 40) ++green;
        if (color.red() > 200 && color.blue() > 200 && color.green() < 40) {
            ++allMagenta;
            std::cout << "RECORDER_VISIBLE frame=" << decoded << " timestamp_us=" << frame.startTime() << '\n';
            if (frame.startTime() < 500000) ++earlyMagenta;
        }
    });
    QObject::connect(&player, &QMediaPlayer::errorOccurred, &app, [&](auto, const QString &message) { fail(message); });
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &app, [&](auto status) {
        if (status != QMediaPlayer::EndOfMedia) return;
        std::cout << "decoded=" << decoded << " green_background=" << green
                  << " initial_recorder_frames=" << earlyMagenta << " total_recorder_frames=" << allMagenta
                  << " duration_ms=" << player.duration() << '\n';
        if (!baseline || decoded < 10 || (optOut ? allMagenta == 0 : (green < 10 || allMagenta > 0))) {
            fail("saved movie did not capture the background after minimization"); return;
        }
        std::cout << "PASS: X11/Openbox actual capture -> minimize -> hotkey pause/resume -> restore/pause -> resume -> hotkey stop -> MP4 decode\n";
        app.exit(0);
    });
    QTimer::singleShot(90000, &app, [&] { fail("desktop recording timed out"); });
    QTimer::singleShot(500, &app, [&] {
        if (MainWindowTestPeer::shortcuts(window) != 3) { fail("global shortcuts not registered"); return; }
        if (direct) baseline = true;
        controller.start(target, options, {}, QMediaFormat::VideoCodec::MPEG4, direct ? output : QString());
    });
    const int result = app.exec();
    controller.stop(); player.stop();
    otherApp.terminate();
    if (!otherApp.waitForFinished(1000)) { otherApp.kill(); otherApp.waitForFinished(); }
    return result;
}
