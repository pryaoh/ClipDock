#include "RecorderController.h"
#include <QFileInfo>
#include <QGuiApplication>
#include <QMediaPlayer>
#include <QPainter>
#include <QScreen>
#include <QTemporaryDir>
#include <iostream>

// Replace only the OS frame producer. Preview, cropping, recorder start/pause/
// stop, temporary-file finalization and decoding all exercise production code.
class RecorderControllerTestPeer {
public:
    static void preview(RecorderController &controller, const RecorderController::Target &target) {
        controller.m_target = target;
        controller.m_options.fps = 25;
        controller.m_regionThrottle.invalidate();
        controller.m_session.setVideoSink(nullptr);
        controller.m_session.setVideoFrameInput(&controller.m_regionInput);
        controller.setState(RecorderController::State::Starting);
    }
    static void frame(RecorderController &controller, const QImage &image) {
        controller.handleRegionFrame(QVideoFrame(image));
    }
    static void captureFailure(RecorderController &controller) {
        controller.fail(QStringLiteral("injected capture interruption"));
    }
    static quint64 waitForPermission(RecorderController &controller, const QString &output) {
        controller.m_outputPath = output;
        controller.m_codec = QMediaFormat::VideoCodec::MPEG4;
        controller.setState(RecorderController::State::WaitingForPermission);
        return ++controller.m_generation;
    }
    static void grantPermission(RecorderController &controller, quint64 generation) {
        controller.completePermissionRequest(generation, true, true);
    }
};

int main(int argc, char **argv) {
    qputenv("QT_MEDIA_BACKEND", QByteArrayLiteral("ffmpeg"));
    QGuiApplication app(argc, argv);
    const bool captureFailure = app.arguments().contains(QStringLiteral("--capture-failure"));
    const bool delayedPermission = app.arguments().contains(QStringLiteral("--delayed-permission"));
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    RecorderController controller;
    QVideoSink preview;
    controller.setVideoSink(&preview);
    RecorderController::Target target;
    target.screen = app.primaryScreen();
    target.isRegion = true;
    target.regionScreenSize = target.screen->size();
    target.region = QRect(20, 30, 160, 90);
    QImage desktop(target.regionScreenSize * 2, QImage::Format_ARGB32);
    desktop.fill(Qt::red); // Must never appear in preview or movie.
    QPainter painter(&desktop);
    painter.fillRect(QRect(40, 60, 320, 180), QColor(0, 220, 0));
    painter.end();
    auto fail = [&](const QString &message) {
        std::cerr << "FAIL: " << message.toStdString() << '\n';
        app.exit(1);
    };
    int captureErrors = 0;
    QObject::connect(&controller, &RecorderController::errorOccurred, &app, [&](const QString &message) {
        if (captureFailure && message == QStringLiteral("injected capture interruption")) ++captureErrors;
        else fail(message);
    });
    int previewFrames = 0;
    QObject::connect(&preview, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid()) return;
        ++previewFrames;
        if (frame.size() != QSize(320, 180) || frame.toImage().pixelColor(0, 0).red() > 20)
            fail(QStringLiteral("preview includes pixels outside region"));
    });
    QTimer producer;
    producer.setTimerType(Qt::PreciseTimer);
    producer.setInterval(40);
    QObject::connect(&producer, &QTimer::timeout, &app, [&] { RecorderControllerTestPeer::frame(controller, desktop); });

    QMediaPlayer player;
    QVideoSink decodedSink;
    player.setVideoSink(&decodedSink);
    int decoded = 0;
    qint64 previous = -1;
    QObject::connect(&decodedSink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid()) return;
        if (frame.size() != QSize(320, 180) || frame.startTime() <= previous)
            fail(QStringLiteral("decoded size or timestamps"));
        previous = frame.startTime();
        const auto image = frame.toImage();
        for (const QPoint &point : {QPoint(0, 0), QPoint(319, 179), QPoint(160, 90)}) {
            const auto color = image.pixelColor(point);
            if (color.red() > 30 || color.green() < 150) fail(QStringLiteral("encoded movie leaks pixels outside selection"));
        }
        ++decoded;
    });
    QObject::connect(&player, &QMediaPlayer::errorOccurred, &app, [&](auto, const QString &message) { fail(message); });
    QObject::connect(&controller, &RecorderController::saved, &app, [&](const QString &path) {
        producer.stop();
        if (controller.state() != RecorderController::State::Stopping) fail(QStringLiteral("unexpected save state"));
        player.setSource(QUrl::fromLocalFile(path));
        player.play();
    });
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &app, [&](auto status) {
        if (status != QMediaPlayer::EndOfMedia) return;
        if (!QFileInfo::exists(directory.filePath(QStringLiteral("region.mp4")))) {
            fail(QStringLiteral("completed recording was removed during temporary-file cleanup"));
            return;
        }
        if (captureFailure && captureErrors != 1) { fail(QStringLiteral("capture failure was not exercised")); return; }
        // 2s recording + 1s paused. Pause must not lengthen the saved movie.
        if (decoded < 20 || previewFrames < 20 || player.duration() < 1500 || player.duration() > 2500) {
            fail(QStringLiteral("frames=%1, preview=%2, duration=%3").arg(decoded).arg(previewFrames).arg(player.duration()));
            return;
        }
        std::cout << "PASS: cropped preview -> MP4 -> decoded pixels; pause/resume; " << decoded
                  << " frames, " << player.duration() << " ms\n";
        app.exit(0);
    });
    bool scheduled = false;
    QObject::connect(&controller, &RecorderController::stateChanged, &app, [&](auto state) {
        if (state != RecorderController::State::Recording || scheduled) return;
        scheduled = true;
        QTimer::singleShot(1000, &controller, &RecorderController::togglePause);
        QTimer::singleShot(2000, &controller, &RecorderController::togglePause);
        QTimer::singleShot(3000, &controller, [&] {
            if (captureFailure) RecorderControllerTestPeer::captureFailure(controller);
            else controller.stop();
        });
    });
    QTimer::singleShot(0, &app, [&] {
        RecorderControllerTestPeer::preview(controller, target);
        RecorderControllerTestPeer::frame(controller, desktop);
        if (controller.state() != RecorderController::State::Preview) { fail(QStringLiteral("preview did not start")); return; }
        RecordingOptions options;
        const auto output = directory.filePath(QStringLiteral("region.mp4"));
        if (delayedPermission) {
            const auto generation = RecorderControllerTestPeer::waitForPermission(controller, output);
            QTimer::singleShot(500, &controller, [&, generation] {
                if (controller.state() != RecorderController::State::WaitingForPermission || previewFrames < 3) {
                    fail(QStringLiteral("preview must continue without recording until permission is granted"));
                    return;
                }
                RecorderControllerTestPeer::grantPermission(controller, generation);
            });
        } else controller.start(target, options, {}, QMediaFormat::VideoCodec::MPEG4, output);
        producer.start();
    });
    QTimer::singleShot(20000, &app, [&] { fail(QStringLiteral("region pipeline timed out")); });
    const int result = app.exec();
    producer.stop();
    player.stop();
    controller.stop();
    return result;
}
