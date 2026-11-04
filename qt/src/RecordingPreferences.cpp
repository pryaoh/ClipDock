#include "RecordingPreferences.h"
#include <QDir>
#include <QFileInfo>
#include <QScreen>
#include <QSettings>
#include <cmath>

ScreenDescriptor ScreenDescriptor::fromScreen(const QScreen *screen) {
    if (!screen) return {};
    return {screen->name(), screen->manufacturer(), screen->model(), screen->serialNumber(),
            screen->size(), screen->devicePixelRatio()};
}

int SavedRegion::matchingScreen(const QList<ScreenDescriptor> &screens) const {
    if (!present || screen.logicalSize.isEmpty() || !std::isfinite(screen.devicePixelRatio)
        || screen.devicePixelRatio <= 0 || rect.x() < 0 || rect.y() < 0 || rect.width() < 16 || rect.height() < 16
        || qint64(rect.x()) + rect.width() > screen.logicalSize.width()
        || qint64(rect.y()) + rect.height() > screen.logicalSize.height()) return -1;
    if (screen.serialNumber.isEmpty() && screen.name.isEmpty()) return -1;
    int match = -1;
    for (qsizetype i = 0; i < screens.size(); ++i) {
        const auto &candidate = screens[i];
        const bool sameIdentity = candidate.manufacturer == screen.manufacturer && candidate.model == screen.model
            && (screen.serialNumber.isEmpty()
                ? candidate.serialNumber.isEmpty() && candidate.name == screen.name
                : candidate.serialNumber == screen.serialNumber);
        if (!sameIdentity) continue;
        if (match >= 0) return -1; // Never disambiguate identical screens by their list order or resolution.
        match = int(i);
    }
    if (match < 0) return -1;
    const auto &candidate = screens[match];
    if (candidate.logicalSize != screen.logicalSize || !std::isfinite(candidate.devicePixelRatio)
        || !qFuzzyCompare(candidate.devicePixelRatio, screen.devicePixelRatio)) return -1;
    return match;
}

RecordingPreferences RecordingPreferences::load(QSettings &store, const QList<QMediaFormat::VideoCodec> &codecs,
                                                 const QList<QByteArray> &audioIds) {
    RecordingPreferences result;
    const auto integer = [&](const QString &key, int fallback, int minimum, int maximum) {
        bool ok = false;
        const qlonglong value = store.value(key, fallback).toLongLong(&ok);
        if (ok && value >= minimum && value <= maximum) return int(value);
        result.usedFallback = true;
        return fallback;
    };
    result.options.maxEdge = integer(QStringLiteral("recording/maxEdge"), 1920, 16, 4096);
    if (!QList<int>{1280, 1920, 2560, 4096}.contains(result.options.maxEdge)) {
        result.options.maxEdge = 1920; result.usedFallback = true;
    }
    result.options.fps = integer(QStringLiteral("recording/fps"), 25, 1, 60);
    if (!QList<int>{10, 15, 25, 30, 60}.contains(result.options.fps)) {
        result.options.fps = 25; result.usedFallback = true;
    }
    result.options.bitrateKbps = integer(QStringLiteral("recording/bitrateKbps"), 5000, 100, 100000);
    const auto codec = static_cast<QMediaFormat::VideoCodec>(integer(QStringLiteral("recording/codec"),
        int(QMediaFormat::VideoCodec::H264), -1, 100));
    // Restrict to the two MP4 codecs exposed by the application, even if the backend offers more.
    QList<QMediaFormat::VideoCodec> available;
    for (auto option : {QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::MPEG4})
        if (codecs.contains(option)) available.append(option);
    if (available.contains(codec)) result.codec = codec;
    else {
        result.codec = available.isEmpty() ? QMediaFormat::VideoCodec::Unspecified : available.first();
        result.usedFallback = true;
    }
    result.audioId = store.value(QStringLiteral("recording/audioId")).toByteArray();
    if (!result.audioId.isEmpty() && !audioIds.contains(result.audioId)) {
        result.audioId.clear(); result.usedFallback = true;
    }
    result.outputFolder = store.value(QStringLiteral("recording/outputFolder")).toString();
    if (!result.outputFolder.isEmpty()) {
        const QFileInfo folder(result.outputFolder);
        if (!folder.isAbsolute() || !folder.isDir() || !folder.isWritable()) {
            result.outputFolder.clear(); result.usedFallback = true;
        }
    }
    result.regionMode = store.value(QStringLiteral("recording/sourceType")).toString() == QStringLiteral("region");
    auto &region = result.region;
    region.present = store.contains(QStringLiteral("lastRegion/rect"));
    region.rect = store.value(QStringLiteral("lastRegion/rect")).toRect();
    region.screen.name = store.value(QStringLiteral("lastRegion/name")).toString();
    region.screen.manufacturer = store.value(QStringLiteral("lastRegion/manufacturer")).toString();
    region.screen.model = store.value(QStringLiteral("lastRegion/model")).toString();
    region.screen.serialNumber = store.value(QStringLiteral("lastRegion/serialNumber")).toString();
    region.screen.logicalSize = store.value(QStringLiteral("lastRegion/logicalSize")).toSize();
    region.screen.devicePixelRatio = store.value(QStringLiteral("lastRegion/devicePixelRatio"), 0).toDouble();
    return result;
}

void RecordingPreferences::save(QSettings &store) const {
    store.setValue(QStringLiteral("recording/maxEdge"), options.maxEdge);
    store.setValue(QStringLiteral("recording/fps"), options.fps);
    store.setValue(QStringLiteral("recording/bitrateKbps"), options.bitrateKbps);
    store.setValue(QStringLiteral("recording/codec"), int(codec));
    store.setValue(QStringLiteral("recording/audioId"), audioId);
    store.setValue(QStringLiteral("recording/outputFolder"), outputFolder);
    store.setValue(QStringLiteral("recording/sourceType"), regionMode ? QStringLiteral("region") : QStringLiteral("screen"));
    if (region.present) {
        store.setValue(QStringLiteral("lastRegion/rect"), region.rect);
        store.setValue(QStringLiteral("lastRegion/name"), region.screen.name);
        store.setValue(QStringLiteral("lastRegion/manufacturer"), region.screen.manufacturer);
        store.setValue(QStringLiteral("lastRegion/model"), region.screen.model);
        store.setValue(QStringLiteral("lastRegion/serialNumber"), region.screen.serialNumber);
        store.setValue(QStringLiteral("lastRegion/logicalSize"), region.screen.logicalSize);
        store.setValue(QStringLiteral("lastRegion/devicePixelRatio"), region.screen.devicePixelRatio);
    } else store.remove(QStringLiteral("lastRegion"));
}

void RecordingPreferences::reset(QSettings &store) {
    store.remove(QStringLiteral("recording"));
    store.remove(QStringLiteral("lastRegion"));
    store.remove(QStringLiteral("capture/excludeSelf"));
    store.remove(QStringLiteral("capture/minimizeRecording"));
}
