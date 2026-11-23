#include "RecorderController.h"
#include <QColor>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include <functional>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMediaPlayer>
#include <QScreen>
#include <QTemporaryDir>
#include <QUrl>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class RecorderControllerTestPeer {
public:
    // Substitute the asynchronous OS permission response; retain production frame handling.
    static quint64 waitForPermission(RecorderController &controller, QScreen *screen,
                                    const QString &output, bool region, int startDelay = 0,
                                    bool fromPreview = true) {
        controller.m_options = {};
        controller.m_options.startDelaySeconds = startDelay;
        // Linux CI's FFmpeg has no H.264 encoder; MPEG-4 is available on every platform.
        controller.m_codec = QMediaFormat::VideoCodec::MPEG4;
        controller.m_countdownFromPreview = fromPreview;
        controller.m_aboutToStartEmitted = false;
        controller.m_writerStarted = false;
        controller.m_target = {};
        controller.m_target.screen = screen;
        controller.m_target.isRegion = region;
        controller.m_target.regionScreenSize = screen->size();
        controller.m_target.region = QRect(0, 0, 160, 90);
        controller.m_outputPath = output;
        controller.m_failed = false;
        controller.m_recorderFailed = false;
        const auto generation = ++controller.m_generation;
        controller.setState(RecorderController::State::WaitingForPermission);
        return generation;
    }
    static void frame(RecorderController &controller, const QImage &image, bool region) {
        Q_UNUSED(region);
        controller.m_captureSink.setVideoFrame(QVideoFrame(image));
    }
    static void answer(RecorderController &controller, quint64 generation, bool granted) {
        controller.completePermissionRequest(generation, true, granted);
    }
    static bool writerStarted(const RecorderController &controller) { return controller.m_writerStarted; }
    static quint64 generation(const RecorderController &controller) { return controller.m_generation; }
    static QColor latestPixel(const RecorderController &controller) {
        return controller.m_latestFrame.toImage().pixelColor(0, 0);
    }
    static void removeScreen(RecorderController &controller) { controller.m_target.screen.clear(); }
    static void shortenStartTimeout(RecorderController &controller) { controller.m_startTimeout.setInterval(100); }
    static QString seedPartialFile(RecorderController &controller, const QString &folder) {
        controller.m_temporary = std::make_unique<QTemporaryFile>(folder + "/.clipdock-XXXXXX.mp4");
        check(controller.m_temporary->open(), "create partial recording");
        controller.m_temporary->write("recoverable recording bytes");
        controller.m_temporary->close();
        controller.m_writerStarted = true;
        controller.m_outputPath = folder + "/final.mp4";
        controller.setState(RecorderController::State::Recording);
        return controller.m_temporary->fileName();
    }
    static void encoderFailure(RecorderController &controller) {
        controller.fail(QStringLiteral("injected encoder error"), true);
    }
};

static bool waitUntil(const std::function<bool()> &condition, int milliseconds) {
    QElapsedTimer clock;
    clock.start();
    while (!condition() && clock.elapsed() < milliseconds) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::msleep(2);
    }
    return condition();
}

static QStringList filesIn(const QString &path) {
    return QDir(path).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
}

static void checkCountdown(QScreen *screen) {
    using State = RecorderController::State;
    QImage countdownFrame(screen->size(), QImage::Format_ARGB32);
    countdownFrame.fill(Qt::red);
    QImage settledFrame(screen->size(), QImage::Format_ARGB32);
    settledFrame.fill(Qt::green);

    {   // Slow permission, re-entrant start, exact single start, settled first frame.
        QTemporaryDir directory;
        check(directory.isValid(), "countdown directory");
        RecorderController controller;
        QStringList errors;
        QList<int> ticks;
        int aboutToStart = 0;
        bool saved = false;
        QObject::connect(&controller, &RecorderController::errorOccurred, [&](const QString &m) { errors.append(m); });
        QObject::connect(&controller, &RecorderController::countdownChanged, [&](int s) { ticks.append(s); });
        QObject::connect(&controller, &RecorderController::saved, [&](const QString &) { saved = true; });
        QObject::connect(&controller, &RecorderController::recordingAboutToStart, [&] {
            ++aboutToStart;
        });
        const auto output = directory.filePath(QStringLiteral("countdown.mp4"));
        const auto generation = RecorderControllerTestPeer::waitForPermission(controller, screen, output, false, 2);
        RecorderControllerTestPeer::frame(controller, countdownFrame, false);
        waitUntil([] { return false; }, 1200); // A slow permission answer must not consume the countdown.
        check(controller.state() == State::WaitingForPermission && ticks.isEmpty(),
              "countdown waits for the permission answer");
        RecorderControllerTestPeer::answer(controller, generation, true);
        QElapsedTimer clock;
        clock.start();
        check(controller.state() == State::Countdown && controller.countdownRemaining() == 2,
              "full countdown starts after permission and first frame");
        check(ticks == QList<int>{2}, "countdown announces the full delay");
        const auto countdownGeneration = RecorderControllerTestPeer::generation(controller);
        RecorderController::Target target;
        target.screen = screen;
        controller.start(target, {}, {}, QMediaFormat::VideoCodec::MPEG4, directory.filePath(QStringLiteral("again.mp4")));
        check(controller.state() == State::Countdown
                  && RecorderControllerTestPeer::generation(controller) == countdownGeneration,
              "a repeated start request during the countdown is ignored");
        check(filesIn(directory.path()).isEmpty(), "countdown creates no files");
        check(waitUntil([&] { return aboutToStart == 1; }, 4000), "countdown prepares the screen before encoding");
        // Capture can stall or deliver a countdown-era image during minimization.
        // Neither the cached image nor an image from inside the settle interval is safe.
        RecorderControllerTestPeer::frame(controller, countdownFrame, false);
        waitUntil([] { return false; }, RecorderController::CountdownSettleMs + 150);
        check(!RecorderControllerTestPeer::writerStarted(controller) && filesIn(directory.path()).isEmpty(),
              "countdown waits for a fresh post-settle frame without creating a file");
        RecorderControllerTestPeer::frame(controller, settledFrame, false);
        check(waitUntil([&] { return RecorderControllerTestPeer::writerStarted(controller); }, 1000),
              "recording starts after the countdown");
        const auto elapsed = clock.elapsed();
        check(elapsed >= 2000 + RecorderController::CountdownSettleMs - 20 && elapsed < 3200,
              "recording starts after the selected delay and settle time");
        check(ticks == (QList<int>{2, 1, 0}), "countdown reports each remaining second, then 0");
        check(aboutToStart == 1, "recording preparation is announced exactly once");
        check(RecorderControllerTestPeer::latestPixel(controller) == QColor(Qt::green),
              "the first recorded frame is captured after the countdown display was removed");
        waitUntil([&] { return controller.state() == State::Recording; }, 3000);
        waitUntil([] { return false; }, 600);
        controller.stop();
        check(waitUntil([&] { return controller.state() == State::Idle; }, 8000), "countdown recording finalizes");
        check(errors.isEmpty(), "countdown recording reports no errors");
        check(saved && filesIn(directory.path()) == QStringList{QStringLiteral("countdown.mp4")},
              "exactly one recording file is saved");
        QMediaPlayer player;
        QVideoSink decoded;
        player.setVideoSink(&decoded);
        int decodedFrames = 0;
        bool stalePixels = false;
        QObject::connect(&decoded, &QVideoSink::videoFrameChanged, [&](const QVideoFrame &frame) {
            if (!frame.isValid()) return;
            const QImage image = frame.toImage();
            check(!image.isNull(), "countdown recording decodes into an image");
            const auto color = image.pixelColor(image.width() / 2, image.height() / 2);
            stalePixels |= color.green() < 180 || color.red() > 60 || color.blue() > 60;
            ++decodedFrames;
        });
        player.setSource(QUrl::fromLocalFile(output));
        player.play();
        check(waitUntil([&] { return player.mediaStatus() == QMediaPlayer::EndOfMedia; }, 5000),
              "countdown recording decodes to the end");
        check(decodedFrames > 0 && !stalePixels, "every recorded frame excludes countdown-era pixels");
        player.stop();
    }

    for (bool fromPreview : {true, false}) {   // Cancellation returns to the prior waiting state.
        QTemporaryDir directory;
        RecorderController controller;
        int aboutToStart = 0, errors = 0;
        QString status;
        QObject::connect(&controller, &RecorderController::recordingAboutToStart, [&] { ++aboutToStart; });
        QObject::connect(&controller, &RecorderController::errorOccurred, [&](const QString &) { ++errors; });
        QObject::connect(&controller, &RecorderController::statusChanged, [&](const QString &m) { status = m; });
        const auto generation = RecorderControllerTestPeer::waitForPermission(
            controller, screen, directory.filePath(QStringLiteral("cancel.mp4")), false, 1, fromPreview);
        RecorderControllerTestPeer::frame(controller, countdownFrame, false);
        RecorderControllerTestPeer::answer(controller, generation, true);
        check(controller.state() == State::Countdown, "cancel test enters countdown");
        controller.stop();
        check(controller.state() == (fromPreview ? State::Preview : State::Idle),
              "cancel returns to the preview or idle state it started from");
        check(controller.countdownRemaining() == 0 && status.contains(QStringLiteral("취소")),
              "cancel clears the countdown and reports it");
        waitUntil([] { return false; }, 1500);
        check(!RecorderControllerTestPeer::writerStarted(controller) && aboutToStart == 0 && errors == 0,
              "a cancelled countdown never starts recording");
        check(filesIn(directory.path()).isEmpty(), "a cancelled countdown leaves no file");
        controller.stop();
        check(controller.state() == State::Idle, "preview can still be ended after cancel");
    }

    for (bool timeout : {false, true}) { // Cancel/timeout while waiting for the fresh post-countdown image.
        QTemporaryDir directory;
        RecorderController controller;
        int errors = 0, prepared = 0;
        QObject::connect(&controller, &RecorderController::errorOccurred, [&](const QString &) { ++errors; });
        QObject::connect(&controller, &RecorderController::recordingAboutToStart, [&] {
            ++prepared;
            if (timeout) RecorderControllerTestPeer::shortenStartTimeout(controller);
        });
        const auto generation = RecorderControllerTestPeer::waitForPermission(
            controller, screen, directory.filePath(QStringLiteral("waiting.mp4")), false, 1);
        RecorderControllerTestPeer::frame(controller, countdownFrame, false);
        RecorderControllerTestPeer::answer(controller, generation, true);
        check(waitUntil([&] { return prepared == 1; }, 2000), "fresh-frame wait follows the countdown");
        if (timeout) {
            check(waitUntil([&] { return controller.state() == State::Idle; }, 2000),
                  "missing post-countdown frame times out");
        } else {
            waitUntil([] { return false; }, RecorderController::CountdownSettleMs + 150);
            check(controller.state() == State::Starting, "fresh-frame wait remains cancellable");
            controller.stop();
        }
        RecorderControllerTestPeer::frame(controller, settledFrame, false);
        waitUntil([] { return false; }, 150);
        check(controller.state() == State::Idle && !RecorderControllerTestPeer::writerStarted(controller)
                  && filesIn(directory.path()).isEmpty() && errors == int(timeout),
              "cancelled or expired fresh-frame waits ignore late images and leave no file");
    }

    {   // The target disappears while counting down.
        QTemporaryDir directory;
        RecorderController controller;
        int errors = 0;
        QObject::connect(&controller, &RecorderController::errorOccurred, [&](const QString &) { ++errors; });
        const auto generation = RecorderControllerTestPeer::waitForPermission(
            controller, screen, directory.filePath(QStringLiteral("gone.mp4")), false, 2);
        RecorderControllerTestPeer::frame(controller, countdownFrame, false);
        RecorderControllerTestPeer::answer(controller, generation, true);
        RecorderControllerTestPeer::removeScreen(controller);
        check(waitUntil([&] { return controller.state() == State::Idle; }, 2000), "missing target ends the countdown");
        check(errors == 1 && !RecorderControllerTestPeer::writerStarted(controller), "missing target is reported, not recorded");
        check(filesIn(directory.path()).isEmpty(), "missing target leaves no file");
    }

    {   // Known output problems are rejected before any permission prompt or countdown.
        QTemporaryDir directory;
        RecorderController controller;
        int errors = 0;
        QObject::connect(&controller, &RecorderController::errorOccurred, [&](const QString &) { ++errors; });
        RecorderController::Target target;
        target.screen = screen;
        RecordingOptions options;
        options.startDelaySeconds = 3;
        controller.start(target, options, {}, QMediaFormat::VideoCodec::MPEG4,
                         directory.filePath(QStringLiteral("missing/folder/out.mp4")));
        check(errors == 1 && controller.state() == State::Idle, "unwritable output folder is rejected immediately");
        options.startDelaySeconds = 11;
        controller.start(target, options, {}, QMediaFormat::VideoCodec::MPEG4, directory.filePath(QStringLiteral("x.mp4")));
        check(errors == 2 && controller.state() == State::Idle, "out-of-range start delay is rejected");
    }
    std::cout << "PASS: countdown permission wait, single start, settled first frame, cancellation, target loss\n";
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QTemporaryDir directory;
    check(directory.isValid(), "temporary directory");
    auto *screen = app.primaryScreen();
    check(screen, "offscreen display");
    if (app.arguments().contains(QStringLiteral("--countdown"))) {
        checkCountdown(screen);
        return 0;
    }
    RecorderController::Target portalTarget;
    portalTarget.isPortal = true;
    check(portalTarget.isAvailable(), "portal target must not require a preselected QScreen");
    portalTarget.isRegion = true;
    check(!portalTarget.isAvailable(), "portal cannot crop using an unrelated monitor's coordinates");
    portalTarget.isRegion = false;
    portalTarget.isWindow = true;
    check(!portalTarget.isAvailable(), "portal cannot use the native window capture path");
    QImage image(screen->size(), QImage::Format_ARGB32);
    image.fill(Qt::green);

    for (bool region : {false, true}) {
        RecorderController controller;
        int errors = 0;
        QObject::connect(&controller, &RecorderController::errorOccurred, &app, [&](const QString &) { ++errors; });
        const auto output = directory.filePath(QStringLiteral("permission.mp4"));
        auto generation = RecorderControllerTestPeer::waitForPermission(controller, screen, output, region);
        RecorderControllerTestPeer::frame(controller, image, region);
        check(controller.state() == RecorderController::State::WaitingForPermission,
              "preview frames must not leave the permission wait");
        check(!RecorderControllerTestPeer::writerStarted(controller), "permission pending must not start writer");
        check(QDir(directory.path()).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty(),
              "permission pending must not create files");
        RecorderControllerTestPeer::answer(controller, generation, false);
        check(errors == 1 && controller.state() == RecorderController::State::Idle, "denial stops capture");

        generation = RecorderControllerTestPeer::waitForPermission(controller, screen, output, region);
        controller.stop();
        const auto next = RecorderControllerTestPeer::waitForPermission(controller, screen, output, region);
        RecorderControllerTestPeer::answer(controller, generation, true);
        check(controller.state() == RecorderController::State::WaitingForPermission,
              "late grant from cancelled operation must not start a new recording");
        check(!RecorderControllerTestPeer::writerStarted(controller), "late grant must not start writer");
        RecorderControllerTestPeer::answer(controller, next, false);
        check(errors == 2 && controller.state() == RecorderController::State::Idle, "new request still accepts denial");

        generation = RecorderControllerTestPeer::waitForPermission(controller, screen, output, region);
        RecorderControllerTestPeer::removeScreen(controller);
        RecorderControllerTestPeer::answer(controller, generation, true);
        check(errors == 3 && controller.state() == RecorderController::State::Idle,
              "screen removed while waiting is rejected after permission grant");
        check(!RecorderControllerTestPeer::writerStarted(controller), "missing target must not start writer");
    }

    RecorderController controller;
    QStringList errors;
    bool saved = false;
    QObject::connect(&controller, &RecorderController::errorOccurred, &app, [&](const QString &message) { errors.append(message); });
    QObject::connect(&controller, &RecorderController::saved, &app, [&](const QString &) { saved = true; });
    const auto temporaryPath = RecorderControllerTestPeer::seedPartialFile(controller, directory.path());
    RecorderControllerTestPeer::encoderFailure(controller);
    QFile retained(temporaryPath);
    check(retained.open(QIODevice::ReadOnly) && retained.readAll() == "recoverable recording bytes",
          "encoder failure retains original recording bytes");
    check(!saved && !QFileInfo::exists(directory.filePath("final.mp4")), "partial movie is not reported as complete");
    check(errors.join('\n').contains(temporaryPath), "recovery file location is reported");
    check(controller.state() == RecorderController::State::Idle, "encoder failure returns to idle");

    RecorderController::Target target;
    check(!target.isAvailable(), "missing screen is rejected");
    target.screen = screen;
    check(target.isAvailable(), "connected screen is available");
    target.isRegion = true;
    target.region = QRect(0, 0, 160, 90);
    target.regionScreenSize = screen->size() + QSize(1, 1);
    check(!target.isAvailable(), "changed screen size invalidates region");
    target.regionScreenSize = screen->size();
    check(target.isAvailable(), "valid region remains available");
    std::cout << "PASS: permission wait/denial/cancellation, partial-file recovery, target validation\n";
}
