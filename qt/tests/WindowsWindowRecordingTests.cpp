#include "RecorderController.h"
#include <QApplication>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QPainter>
#include <QTemporaryDir>
#include <QEvent>
#include <QWidget>
#include <QUrl>
#include <cmath>
#include <iostream>
#include <functional>

namespace {
class StaticWindow final : public QWidget {
public:
    QColor color = Qt::green;
    void paintEvent(QPaintEvent *) override { QPainter(this).fillRect(rect(), color); }
};
}

class CancelQueuedImage final : public QObject {
public:
    std::function<void()> cancel;
    bool intercepted = false;
    bool eventFilter(QObject *, QEvent *event) override {
        // The bootstrap posts a MetaCall only after obtaining a non-null OS
        // image. Cancel before that callback executes, not merely after the
        // worker exits (which could also indicate capture failure).
        if (!intercepted && event->type() == QEvent::MetaCall) {
            intercepted = true;
            cancel();
        }
        return false;
    }
};

// Opt-in desktop regression: do not repaint, move or resize the source to
// manufacture the first frame. Only the OS capture producer is used.
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    std::cout << std::unitbuf;
    const bool pulse = app.arguments().contains(QStringLiteral("--pulse"));
    const int fps = app.arguments().contains(QStringLiteral("--fps60")) ? 60 : (pulse ? 25 : 30);
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    const QString output = directory.filePath(QStringLiteral("static-window.mp4"));
    StaticWindow source;
    source.setWindowTitle(QStringLiteral("ClipDock native static window regression %1").arg(QCoreApplication::applicationPid()));
    source.resize(640, 360);
    source.show();
    if (app.arguments().contains(QStringLiteral("--cancel-restart"))) {
        StaticWindow replacement;
        replacement.color = Qt::blue;
        replacement.setWindowTitle(source.windowTitle() + QStringLiteral(" replacement"));
        replacement.resize(640, 360); replacement.show();
        WindowCaptureBootstrap bootstrap;
        CancelQueuedImage queuedImage;
        bootstrap.installEventFilter(&queuedImage);
        int received = 0;
        bool failed = false;
        QObject::connect(&bootstrap, &WindowCaptureBootstrap::frameReady, &app, [&](const QImage &image) {
            ++received;
            const auto color = image.pixelColor(image.width() / 2, image.height() / 2);
            if (!queuedImage.intercepted || received != 1 || color.blue() < 180 || color.red() > 60 || color.green() > 60) {
                failed = true;
                std::cerr << "FAIL: canceled capture delivered an old frame after restart\n";
                app.exit(1);
            } else {
                std::cout << "PASS: canceled queued first frame is ignored; replacement window supplies its own image\n";
                app.exit(0);
            }
        });
        QTimer::singleShot(700, &app, [&] {
            QCapturableWindow original, next;
            for (const auto &window : QWindowCapture::capturableWindows()) {
                if (window.description() == source.windowTitle()) original = window;
                if (window.description() == replacement.windowTitle()) next = window;
            }
            if (!original.isValid() || !next.isValid()) { failed = true; app.exit(1); return; }
            queuedImage.cancel = [&, next] { bootstrap.stop(); bootstrap.start(next); };
            bootstrap.start(original);
        });
        QTimer::singleShot(10000, &app, [&] {
            std::cerr << "FAIL: replacement capture timed out\n";
            failed = true; app.exit(1);
        });
        const int result = app.exec();
        bootstrap.stop();
        return failed ? 1 : result;
    }
    RecorderController controller;
    RecordingOptions options;
    options.microphone = false; options.fps = fps; options.maxEdge = 1280;
    QVideoSink preview, decoded;
    controller.setVideoSink(&preview);
    QMediaPlayer player;
    player.setVideoSink(&decoded);
    int failures = 0, recordingStates = 0, pauses = 0, frames = 0, green = 0, blue = 0;
    qint64 previous = -1, firstBlue = -1, activeMs = 0;
    QElapsedTimer activeClock;
    bool saved = false;
    QSize recordedSize;
    auto fail = [&](const QString &message) {
        ++failures;
        std::cerr << "FAIL: " << message.toStdString() << '\n';
        app.exit(1);
    };
    QObject::connect(&controller, &RecorderController::errorOccurred, &app, fail);
    QObject::connect(&controller, &RecorderController::saved, &app, [&](const QString &path) {
        saved = path == output && QFileInfo(path).size() > 0;
    });
    QObject::connect(&controller, &RecorderController::stateChanged, &app, [&](auto state) {
        using State = RecorderController::State;
        if (state == State::Recording) {
            activeClock.restart();
            if (++recordingStates == 1) {
                if (pulse) QTimer::singleShot(1000, &app, [&] { source.color = Qt::blue; source.update(); });
                QTimer::singleShot(2000, &controller, &RecorderController::togglePause);
            } else if (recordingStates == 2) {
                QTimer::singleShot(2000, &controller, &RecorderController::stop);
            } else fail(QStringLiteral("unexpected additional recording state"));
        } else if (state == State::Paused) {
            activeMs += activeClock.elapsed(); ++pauses;
            QTimer::singleShot(1000, &controller, &RecorderController::togglePause);
        } else if (state == State::Stopping) {
            activeMs += activeClock.elapsed();
        } else if (state == State::Idle && saved) {
            source.hide();
            player.setSource(QUrl::fromLocalFile(output)); player.play();
        }
    });
    QObject::connect(&decoded, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid()) return;
        if (frame.startTime() <= previous || qAbs(frame.startTime() - frames * 1000000LL / fps) > 1000) {
            fail(QStringLiteral("decoded frame presentation timestamp does not match the selected FPS")); return;
        }
        const QImage image = frame.toImage();
        if (image.isNull() || image.width() < 600 || image.height() < 350
            || image.width() % 2 || image.height() % 2) {
            fail(QStringLiteral("invalid decoded window image or encoder size")); return;
        }
        if (recordedSize.isEmpty()) recordedSize = image.size();
        if (image.size() != recordedSize) { fail(QStringLiteral("output size changed while recording")); return; }
        const auto color = image.pixelColor(image.width() / 2, image.height() / 2);
        if (color.green() > 180 && color.red() < 60 && color.blue() < 60) ++green;
        else if (color.blue() > 180 && color.red() < 60 && color.green() < 60) {
            ++blue;
            if (firstBlue < 0) firstBlue = frame.startTime();
        } else { fail(QStringLiteral("decoded frame does not contain the selected source window")); return; }
        previous = frame.startTime(); ++frames;
    });
    QObject::connect(&player, &QMediaPlayer::errorOccurred, &app, [&](auto, const QString &message) { fail(message); });
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &app, [&](auto status) {
        if (status != QMediaPlayer::EndOfMedia) return;
        const double actualFps = frames * 1000.0 / player.duration();
        std::cout << "frames=" << frames << " duration_ms=" << player.duration()
                  << " active_ms=" << activeMs << " fps=" << actualFps
                  << " size=" << recordedSize.width() << 'x' << recordedSize.height()
                  << " first_blue_us=" << firstBlue << '\n';
        if (!saved || recordingStates != 2 || pauses != 1 || frames < fps * 3
            || green < fps / 2 || std::abs(actualFps - fps) > 0.15
            || qAbs(player.duration() - activeMs) > 250
            || (pulse && (blue < fps || firstBlue < 800000 || firstBlue > 1500000))) {
            fail(QStringLiteral("window recording duration, FPS, pause exclusion or color transition mismatch")); return;
        }
        std::cout << "PASS: initially static window -> real capture -> MP4 -> full decode; pause/resume and FPS\n";
        app.exit(0);
    });
    QTimer::singleShot(700, &app, [&] {
        RecorderController::Target target;
        target.isWindow = true;
        for (const auto &window : QWindowCapture::capturableWindows())
            if (window.description() == source.windowTitle()) target.window = window;
        if (!target.isAvailable()) { fail(QStringLiteral("test window cannot be enumerated")); return; }
        controller.start(target, options, {}, QMediaFormat::VideoCodec::H264, output);
    });
    QTimer::singleShot(35000, &app, [&] { fail(QStringLiteral("native window recording timed out")); });
    const int result = app.exec();
    player.stop(); controller.stop();
    return failures ? 1 : result;
}
