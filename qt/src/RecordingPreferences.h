#pragma once
#include "RecordingOptions.h"
#include <QByteArray>
#include <QList>
#include <QMediaFormat>
#include <QRect>
#include <QString>

class QScreen;
class QSettings;

struct ScreenDescriptor {
    QString name;
    QString manufacturer;
    QString model;
    QString serialNumber;
    QSize logicalSize;
    qreal devicePixelRatio = 1.0;
    static ScreenDescriptor fromScreen(const QScreen *screen);
};

struct SavedRegion {
    ScreenDescriptor screen;
    QRect rect;
    bool present = false;
    // Unique identity AND unchanged local coordinate space are required.
    int matchingScreen(const QList<ScreenDescriptor> &screens) const;
};

struct RecordingPreferences {
    RecordingOptions options;
    QMediaFormat::VideoCodec codec = QMediaFormat::VideoCodec::H264;
    QByteArray audioId;
    QString outputFolder;
    bool regionMode = false;
    SavedRegion region;
    bool usedFallback = false;

    // Start delays offered in the UI; anything else in the store falls back to no delay.
    static QList<int> startDelayChoices() { return {0, 3, 5}; }

    static RecordingPreferences load(QSettings &store, const QList<QMediaFormat::VideoCodec> &codecs,
                                     const QList<QByteArray> &audioIds);
    void save(QSettings &store) const;
    static void reset(QSettings &store);
};
