#include "MediaFormat.h"
#include "RecordingOptions.h"
#include <QMediaRecorder>

QMediaFormat recordingFormat(QMediaFormat::VideoCodec codec, bool audio) {
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(codec);
    format.setAudioCodec(audio ? QMediaFormat::AudioCodec::AAC : QMediaFormat::AudioCodec::Unspecified);
    return format;
}

void configureRecorder(QMediaRecorder &recorder, const RecordingOptions &options,
                       const QSize &size, QMediaFormat::VideoCodec codec) {
    recorder.setMediaFormat(recordingFormat(codec, options.microphone));
    recorder.setEncodingMode(QMediaRecorder::AverageBitRateEncoding);
    recorder.setVideoResolution(size);
    recorder.setVideoFrameRate(options.fps);
    recorder.setVideoBitRate(options.bitrateKbps * 1000);
    if (options.microphone) {
        recorder.setAudioSampleRate(48000);
        recorder.setAudioChannelCount(2);
        recorder.setAudioBitRate(192000);
    }
}
