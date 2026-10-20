#pragma once
#include <QMediaFormat>
#include <QSize>
class QMediaRecorder;
struct RecordingOptions;

QMediaFormat recordingFormat(QMediaFormat::VideoCodec codec, bool audio);
void configureRecorder(QMediaRecorder &recorder, const RecordingOptions &options,
                       const QSize &size, QMediaFormat::VideoCodec codec);
