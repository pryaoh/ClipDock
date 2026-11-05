#pragma once
#include "RecordingOptions.h"
#include "WindowCaptureBootstrap.h"
#include <QAudioDevice>
#include <QAudioInput>
#include <QCapturableWindow>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QImage>
#include <QPointer>
#include <QScreenCapture>
#include <QTemporaryFile>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoFrameInput>
#include <QVideoSink>
#include <QElapsedTimer>
#include <QWindowCapture>
#include <QtGui/qwindowdefs.h>
#include <memory>
#include <functional>

class QScreen;
class QVideoSink;

class RecorderController final : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, WaitingForPermission, Starting, Preview, Recording, Paused, Stopping };
    Q_ENUM(State)
    struct Target {
        QPointer<QScreen> screen;
        QCapturableWindow window;
        bool isWindow = false;
        QRect region;
        QSize regionScreenSize;
        bool isRegion = false;
        // Wayland selects the source in the OS portal after capture starts.
        bool isPortal = false;
        bool isAvailable() const;
    };

    explicit RecorderController(QObject *parent = nullptr);
    ~RecorderController() override;
    void setVideoSink(QVideoSink *sink);
    void setCaptureExclusionProvider(std::function<QList<WId>()> provider);
    State state() const { return m_state; }
    const Target &target() const { return m_target; }
    void start(const Target &target, const RecordingOptions &options, const QAudioDevice &audio,
               QMediaFormat::VideoCodec codec, const QString &outputPath = {});
    void togglePause();
    void stop();

signals:
    void recordingAboutToStart(); // Permissions, first frame and output preparation are complete.
    void stateChanged(State state);
    void durationChanged(qint64 milliseconds);
    void statusChanged(const QString &message);
    void errorOccurred(const QString &message);
    void saved(const QString &path);

private:
    friend class RecorderControllerTestPeer; // Synthetic source for permission-free pipeline regression tests.
    void setState(State state);
    void completePermissionRequest(quint64 generation, bool hadPreview, bool granted);
    void activateCapture();
    void startWriter();
    void handleFrame(const QVideoFrame &frame);
    void handleCapturedFrame(const QVideoFrame &frame);
    void sendScheduledFrame();
    void sendInputFrame(QVideoFrame frame);
    void captureProtectedFrame();
    void handleRecorderState(QMediaRecorder::RecorderState state);
    void fail(const QString &message, bool recorderError = false);
    void finish();
    void stopCapture();

    QMediaCaptureSession m_session;
    QMediaCaptureSession m_captureSession;
    QVideoSink m_captureSink;
    QVideoFrameInput m_frameInput;
    QElapsedTimer m_recordingClock;
    QTimer m_frameTimer;
    QVideoFrame m_latestFrame;
    QImage m_latestImage;
    qint64 m_nextFrame = 0;
    qint64 m_pausedAt = -1;
    qint64 m_pausedDuration = 0;
    QMetaObject::Connection m_regionGeometryConnection;
    QMediaRecorder m_recorder;
    QScreenCapture m_screen;
    QWindowCapture m_window;
    WindowCaptureBootstrap m_windowBootstrap;
    QAudioInput m_audio;
    QTimer m_startTimeout;
    QTimer m_protectedCaptureTimer;
    std::function<QList<WId>()> m_excludedWindows;
    bool m_protectedCapture = false;
    bool m_protectedNeedsSync = true;
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
    bool m_recorderFailed = false;
    bool m_writerStarted = false;
    quint64 m_generation = 0;
};
