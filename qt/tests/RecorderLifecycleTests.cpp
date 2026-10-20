#include "RecorderController.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QTemporaryDir>
#include <cstdlib>
#include <iostream>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class RecorderControllerTestPeer {
public:
    // Substitute the asynchronous OS permission response; retain production frame handling.
    static quint64 waitForPermission(RecorderController &controller, QScreen *screen,
                                    const QString &output, bool region) {
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
        if (region) controller.handleRegionFrame(QVideoFrame(image));
        else controller.handleFrame(QVideoFrame(image));
    }
    static void answer(RecorderController &controller, quint64 generation, bool granted) {
        controller.completePermissionRequest(generation, true, granted);
    }
    static bool writerStarted(const RecorderController &controller) { return controller.m_writerStarted; }
    static void removeScreen(RecorderController &controller) { controller.m_target.screen.clear(); }
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

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QTemporaryDir directory;
    check(directory.isValid(), "temporary directory");
    auto *screen = app.primaryScreen();
    check(screen, "offscreen display");
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
