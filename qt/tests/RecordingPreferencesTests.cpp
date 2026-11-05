#include "RecordingPreferences.h"
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QFile>
#include <cstdlib>
#include <iostream>
#include <limits>

static void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    check(directory.isValid(), "temporary settings directory");
    const auto file = directory.filePath(QStringLiteral("settings.ini"));
    using Codec = QMediaFormat::VideoCodec;
    const QList<Codec> codecs{Codec::H264, Codec::MPEG4};
    const QList<QByteArray> inputs{QByteArrayLiteral("microphone-B"), QByteArrayLiteral("microphone-A")};
    ScreenDescriptor a{QStringLiteral("DP-1"), QStringLiteral("Vendor"), QStringLiteral("Panel"),
                       QStringLiteral("serial-A"), QSize(1920, 1080), 1.5};
    ScreenDescriptor b = a;
    b.name = QStringLiteral("DP-2"); b.serialNumber = QStringLiteral("serial-B");
    {
        QSettings store(file, QSettings::IniFormat);
        auto defaults = RecordingPreferences::load(store, codecs, inputs);
        check(defaults.options.maxEdge == 1920 && defaults.options.fps == 25
              && defaults.options.bitrateKbps == 5000 && defaults.audioId.isEmpty(), "default values");
        defaults.options.maxEdge = 2560; defaults.options.fps = 60; defaults.options.bitrateKbps = 12345;
        defaults.codec = Codec::MPEG4; defaults.audioId = inputs[1]; defaults.outputFolder = directory.path();
        defaults.regionMode = true;
        defaults.region = {a, QRect(10, 20, 300, 200), true};
        defaults.save(store);
        store.sync();
    }
    QSettings store(file, QSettings::IniFormat);
    auto restored = RecordingPreferences::load(store, codecs, inputs);
    check(restored.options.maxEdge == 2560 && restored.options.fps == 60
          && restored.options.bitrateKbps == 12345 && restored.codec == Codec::MPEG4,
          "quality values survive storage recreation");
    check(restored.audioId == inputs[1] && restored.outputFolder == directory.path() && restored.regionMode,
          "audio uses device identity and folder/mode round-trip");
    check(restored.region.matchingScreen({b, a}) == 1, "monitor order does not change region identity");
    check(restored.region.rect == QRect(10, 20, 300, 200), "screen-local region round-trip");
    check(restored.region.matchingScreen({b}) == -1, "missing screen is not replaced by another monitor");
    check(restored.region.matchingScreen({a, a}) == -1, "duplicate serial numbers require reselection");
    auto changed = a; changed.name = QStringLiteral("DP-9");
    check(restored.region.matchingScreen({changed}) == 0, "serial identity survives port name change");
    changed = a; changed.logicalSize = QSize(1280, 720);
    check(restored.region.matchingScreen({changed}) == -1, "resolution changes require reselection");
    changed = a; changed.devicePixelRatio = 2.0;
    check(restored.region.matchingScreen({changed}) == -1, "scale-only changes require reselection");
    auto region = restored.region;
    region.screen.serialNumber.clear(); a.serialNumber.clear();
    check(region.matchingScreen({b, a}) == 1, "unique screen metadata works without serial");
    check(region.matchingScreen({a, a}) == -1, "identical unnumbered screens are ambiguous");
    region.screen.name.clear(); region.screen.manufacturer.clear(); region.screen.model.clear();
    check(region.matchingScreen({region.screen}) == -1, "anonymous screens cannot be restored");
    region = restored.region; region.rect = QRect(-10, 0, 50, 50);
    check(region.matchingScreen({restored.region.screen}) == -1, "negative local coordinates rejected");
    region.rect = QRect(1910, 0, 50, 50);
    check(region.matchingScreen({restored.region.screen}) == -1, "out-of-bounds crop rejected");
    region.rect = QRect(0, 0, 15, 50);
    check(region.matchingScreen({restored.region.screen}) == -1, "tiny region rejected");
    region.rect = QRect(0, 0, 50, 50); region.screen.devicePixelRatio = std::numeric_limits<double>::quiet_NaN();
    check(region.matchingScreen({region.screen}) == -1, "nonfinite scale rejected");
    auto unavailable = RecordingPreferences::load(store, {Codec::H264}, {});
    check(unavailable.codec == Codec::H264 && unavailable.audioId.isEmpty() && unavailable.usedFallback,
          "missing audio and unsupported codec use safe available defaults");
    check(RecordingPreferences::load(store, {}, {}).codec == Codec::Unspecified, "no encoder is explicit");
    store.setValue(QStringLiteral("recording/fps"), qlonglong(4294967296LL + 25));
    check(RecordingPreferences::load(store, codecs, inputs).usedFallback,
          "overflowing integers must not wrap into a valid choice");
    store.setValue(QStringLiteral("recording/fps"), QStringLiteral("broken"));
    store.setValue(QStringLiteral("recording/maxEdge"), -10);
    store.setValue(QStringLiteral("recording/bitrateKbps"), std::numeric_limits<qlonglong>::max());
    store.setValue(QStringLiteral("recording/codec"), 999);
    store.setValue(QStringLiteral("recording/outputFolder"), directory.filePath(QStringLiteral("missing")));
    auto invalid = RecordingPreferences::load(store, codecs, inputs);
    check(invalid.options.fps == 25 && invalid.options.maxEdge == 1920 && invalid.options.bitrateKbps == 5000
          && invalid.codec == Codec::H264 && invalid.outputFolder.isEmpty() && invalid.usedFallback,
          "corrupt fields recover independently");
    store.setValue(QStringLiteral("capture/excludeSelf"), false);
    store.setValue(QStringLiteral("capture/minimizeRecording"), true);
    store.setValue(QStringLiteral("unrelated/value"), 42);
    RecordingPreferences::reset(store);
    auto reset = RecordingPreferences::load(store, codecs, inputs);
    check(!reset.region.present && !reset.regionMode && reset.options.fps == 25 && reset.audioId.isEmpty()
          && reset.outputFolder.isEmpty(), "reset removes recording preferences and region");
    check(!store.contains(QStringLiteral("capture/excludeSelf")) && !store.contains(QStringLiteral("capture/minimizeRecording"))
          && store.value(QStringLiteral("unrelated/value")).toInt() == 42, "reset only removes owned preferences");
    std::cout << "PASS: settings round-trip, invalid values, missing devices, monitor matching and reset\n";
}
