#include "MediaFormat.h"
#include "RecordingOptions.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMediaCaptureSession>
#include <QMediaPlayer>
#include <QMediaRecorder>
#include <QPainter>
#include <QTemporaryDir>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoFrameInput>
#include <QVideoSink>
#include <iostream>

// An actual encode -> file -> decode test, with no screen or microphone permissions.
int main(int argc, char **argv) {
    qputenv("QT_MEDIA_BACKEND", QByteArrayLiteral("ffmpeg"));
    QGuiApplication app(argc, argv);
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    const auto path = directory.filePath(QStringLiteral("smoke.mp4"));
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    QVideoFrameFormat frameFormat(QSize(320, 180), QVideoFrameFormat::Format_BGRA8888);
    frameFormat.setStreamFrameRate(25);
    QVideoFrameInput input(frameFormat);
    session.setVideoFrameInput(&input);
    session.setRecorder(&recorder);
    RecordingOptions options;
    // The software MPEG-4 encoder avoids requiring hardware acceleration in headless CI.
    configureRecorder(recorder, options, QSize(320, 180), QMediaFormat::VideoCodec::MPEG4);
    recorder.setOutputLocation(QUrl::fromLocalFile(path));
    recorder.setAutoStop(true);

    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &app, [&] {
        std::cerr << "FAIL: media test timed out\n";
        app.exit(1);
    });
    deadline.start(35000);
    QObject::connect(&recorder, &QMediaRecorder::errorOccurred, &app, [&](auto, const QString &error) {
        std::cerr << "FAIL encoding: " << error.toStdString() << '\n';
        app.exit(1);
    });
    int submitted = 0;
    bool ended = false;
    bool recordingStarted = false;
    QTimer producer;
    producer.setInterval(5);
    QObject::connect(&producer, &QTimer::timeout, &app, [&] {
        if (ended || recorder.recorderState() != QMediaRecorder::RecordingState) return;
        if (submitted == 50) {
            ended = input.sendVideoFrame(QVideoFrame());
            if (ended) producer.stop();
            return;
        }
        QImage image(320, 180, QImage::Format_ARGB32);
        image.fill(QColor::fromHsv((submitted * 7) % 360, 180, 180));
        QPainter painter(&image);
        painter.setPen(Qt::white);
        painter.drawText(image.rect(), Qt::AlignCenter, QStringLiteral("Frame %1").arg(submitted));
        painter.end();
        QVideoFrame frame(image);
        frame.setStartTime(submitted * 40000LL);
        frame.setEndTime((submitted + 1) * 40000LL);
        if (input.sendVideoFrame(frame)) ++submitted;
    });

    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    int decoded = 0;
    qint64 previousTime = -1;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid()) return;
        if (frame.size() != QSize(320, 180) || frame.startTime() <= previousTime) {
            std::cerr << "FAIL: decoded dimensions/timestamps\n";
            app.exit(1);
            return;
        }
        previousTime = frame.startTime();
        ++decoded;
    });
    QObject::connect(&player, &QMediaPlayer::errorOccurred, &app, [&](auto, const QString &error) {
        std::cerr << "FAIL decoding: " << error.toStdString() << '\n';
        app.exit(1);
    });
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &app, [&](auto status) {
        if (status != QMediaPlayer::EndOfMedia) return;
        if (decoded < 40 || player.duration() < 1900 || player.duration() > 2100) {
            std::cerr << "FAIL: frames=" << decoded << " duration=" << player.duration() << '\n';
            app.exit(1);
            return;
        }
        std::cout << "PASS MP4 encode/decode: " << decoded << " frames, " << player.duration() << " ms\n";
        app.exit(0);
    });
    QObject::connect(&recorder, &QMediaRecorder::recorderStateChanged, &app, [&](auto state) {
        if (state == QMediaRecorder::RecordingState) {
            recordingStarted = true;
            producer.start();
        }
        if (state == QMediaRecorder::StoppedState && recordingStarted) {
            if (!ended || QFileInfo(path).size() < 1024) {
                std::cerr << "FAIL: output movie was not finalized\n";
                app.exit(1);
                return;
            }
            player.setSource(QUrl::fromLocalFile(path));
            player.play();
        }
    });
    QTimer::singleShot(0, &recorder, &QMediaRecorder::record);
    const int result = app.exec();
    player.stop();
    recorder.stop();
    session.setRecorder(nullptr);
    session.setVideoFrameInput(nullptr);
    return result;
}
