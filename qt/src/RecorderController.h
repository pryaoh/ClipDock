#pragma once
#include "RecordingOptions.h"
#include <QAudioDevice>
#include <QAudioInput>
#include <QCapturableWindow>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QPointer>
#include <QScreenCapture>
#include <QTemporaryFile>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoFrameInput>
#include <QVideoSink>
#include <QElapsedTimer>
#include <QWindowCapture>
#include <memory>

class QScreen;
class QVideoSink;

class RecorderController final : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Starting, Preview, Recording, Paused, Stopping };
    Q_ENUM(State)
    struct Target {
        QPointer<QScreen> screen;
        QCapturableWindow window;
        bool isWindow = false;
        QRect region;
        QSize regionScreenSize;
        bool isRegion = false;
    };

    explicit RecorderController(QObject *parent = nullptr);
    ~RecorderController() override;
    void setVideoSink(QVideoSink *sink);
    State state() const { return m_state; }
    void start(const Target &target, const RecordingOptions &options, const QAudioDevice &audio,
               QMediaFormat::VideoCodec codec, const QString &outputPath = {});
    void togglePause();
    void stop();

signals:
    void stateChanged(State state);
    void durationChanged(qint64 milliseconds);
    void statusChanged(const QString &message);
    void errorOccurred(const QString &message);
    void saved(const QString &path);

private:
    friend class RecorderControllerTestPeer; // Synthetic source for permission-free pipeline regression tests.
    void setState(State state);
    void activateCapture();
    void startWriter();
    void handleFrame(const QVideoFrame &frame);
    void handleRegionFrame(const QVideoFrame &frame);
    void handleRecorderState(QMediaRecorder::RecorderState state);
    void fail(const QString &message);
    void finish();
    void stopCapture();

    QMediaCaptureSession m_session;
    QMediaCaptureSession m_regionSession;
    QVideoSink m_regionSink;
    QVideoFrameInput m_regionInput;
    QElapsedTimer m_regionClock;
    QElapsedTimer m_regionThrottle;
    qint64 m_regionPausedAt = -1;
    qint64 m_regionPausedDuration = 0;
    QMetaObject::Connection m_regionGeometryConnection;
    QMediaRecorder m_recorder;
    QScreenCapture m_screen;
    QWindowCapture m_window;
    QAudioInput m_audio;
    QTimer m_startTimeout;
    QPointer<QVideoSink> m_sink;
    std::unique_ptr<QTemporaryFile> m_temporary;
    State m_state = State::Idle;
    Target m_target;
    RecordingOptions m_options;
    QAudioDevice m_audioDevice;
    QMediaFormat::VideoCodec m_codec = QMediaFormat::VideoCodec::H264;
    QString m_outputPath;
    QSize m_sourceSize;
    bool m_failed = false;
    bool m_writerStarted = false;
    quint64 m_generation = 0;
};
