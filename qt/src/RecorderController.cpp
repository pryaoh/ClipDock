#include "RecorderController.h"
#include "MediaFormat.h"
#include "PlatformPermissions.h"
#include "RegionCapture.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPermission>
#include <QScreen>
#include <QUrl>
#include <QVideoSink>

bool RecorderController::Target::isAvailable() const {
    if (isPortal) return !isWindow && !isRegion;
    if (isWindow) return !isRegion && window.isValid();
    if (!screen || !QGuiApplication::screens().contains(screen.data())) return false;
    return !isRegion || (screen->size() == regionScreenSize
        && !regionPixelRect(region, regionScreenSize, regionScreenSize).isEmpty());
}

RecorderController::RecorderController(QObject *parent) : QObject(parent) {
    m_session.setRecorder(&m_recorder);
    m_regionSession.setVideoSink(&m_regionSink);
    connect(&m_regionSink, &QVideoSink::videoFrameChanged, this, &RecorderController::handleRegionFrame);
    m_startTimeout.setSingleShot(true);
    m_startTimeout.setInterval(20000);
    connect(&m_startTimeout, &QTimer::timeout, this, [this] {
        fail(QStringLiteral("캡처가 시작되지 않았습니다. 화면 권한과 선택한 화면/창을 확인하세요."));
    });
    connect(&m_screen, &QScreenCapture::errorOccurred, this, [this](auto, const QString &text) { fail(text); });
    connect(&m_window, &QWindowCapture::errorOccurred, this, [this](auto, const QString &text) { fail(text); });
    connect(&m_recorder, &QMediaRecorder::errorOccurred, this, [this](auto, const QString &text) {
        fail(text + QStringLiteral("\nH.264 인코더를 사용할 수 없다면 MPEG-4 코덱을 선택해 보세요."), true);
    });
    connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this, &RecorderController::handleRecorderState);
    connect(&m_recorder, &QMediaRecorder::durationChanged, this, &RecorderController::durationChanged);
}

RecorderController::~RecorderController() {
    // MainWindow defers close until StoppedState, which means the container has been finalized.
    m_session.setRecorder(nullptr);
    m_regionSession.setScreenCapture(nullptr);
    m_regionSession.setVideoSink(nullptr);
    m_session.setVideoFrameInput(nullptr);
    m_session.setScreenCapture(nullptr);
    m_session.setWindowCapture(nullptr);
    m_session.setAudioInput(nullptr);
    m_session.setVideoSink(nullptr);
}

void RecorderController::setVideoSink(QVideoSink *sink) {
    if (m_sink) disconnect(m_sink, nullptr, this, nullptr);
    m_sink = sink;
    m_session.setVideoSink(sink);
    connect(sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (!m_target.isRegion) handleFrame(frame);
    });
}

void RecorderController::setState(State state) {
    if (m_state == state) return;
    m_state = state;
    emit stateChanged(state);
}

void RecorderController::start(const Target &target, const RecordingOptions &options,
                               const QAudioDevice &audio, QMediaFormat::VideoCodec codec,
                               const QString &outputPath) {
    if (m_state != State::Idle && m_state != State::Preview) return;
    if (!target.isAvailable()) {
        emit errorOccurred(QStringLiteral("녹화 대상을 다시 선택하세요. 창이 닫혔거나 디스플레이 구성이 변경되었을 수 있습니다."));
        return;
    }
    if (const auto error = options.validate()) {
        emit errorOccurred(QString::fromStdString(*error));
        return;
    }
    if (!outputPath.isEmpty() && QFileInfo::exists(outputPath)) {
        emit errorOccurred(QStringLiteral("기존 파일을 덮어쓰지 않습니다. 다른 이름을 선택하세요."));
        return;
    }
    if (!outputPath.isEmpty() && options.microphone && audio.isNull()) {
        emit errorOccurred(QStringLiteral("사용 가능한 오디오 입력 장치가 없습니다."));
        return;
    }
    m_options = options;
    m_audioDevice = audio;
    m_codec = codec;
    m_outputPath = outputPath;
    m_failed = false;
    m_recorderFailed = false;
    m_writerStarted = false;
    const bool hadPreview = m_state == State::Preview;
    if (!hadPreview) {
        m_target = target;
        m_sourceSize = {};
    }
    const auto generation = ++m_generation;
    // Preview frames keep arriving while the OS permission prompt is open.
    // Only Starting may turn those frames into a recording.
    setState(State::WaitingForPermission);
    emit durationChanged(0);
    if (generation != m_generation || m_state != State::WaitingForPermission) return;
    if (!outputPath.isEmpty() && options.microphone) {
        const QMicrophonePermission permission;
        const auto status = qApp->checkPermission(permission);
        if (status == Qt::PermissionStatus::Undetermined) {
            emit statusChanged(QStringLiteral("마이크 권한 승인 대기 중…"));
            qApp->requestPermission(permission, this, [this, generation, hadPreview](const QPermission &result) {
                completePermissionRequest(generation, hadPreview, result.status() == Qt::PermissionStatus::Granted);
            });
            return;
        }
        if (status != Qt::PermissionStatus::Granted) {
            completePermissionRequest(generation, hadPreview, false);
            return;
        }
    }
    completePermissionRequest(generation, hadPreview, true);
}

void RecorderController::completePermissionRequest(quint64 generation, bool hadPreview, bool granted) {
    if (generation != m_generation || m_state != State::WaitingForPermission) return;
    if (!granted) {
        fail(QStringLiteral("마이크 권한이 거부되었습니다. OS 개인정보 설정에서 허용하세요."));
        return;
    }
    if (!m_target.isAvailable()) {
        fail(QStringLiteral("녹화 대상이 변경되었거나 사라졌습니다. 대상을 다시 선택하세요."));
        return;
    }
    setState(State::Starting);
    if (hadPreview) startWriter();
    else activateCapture();
}

void RecorderController::activateCapture() {
    QString permissionMessage;
    if (!requestScreenPermission(permissionMessage)) { fail(permissionMessage); return; }
    emit statusChanged(QStringLiteral("캡처 시작 중… OS 화면 선택 창이 나타나면 대상을 선택하세요."));
    // Portal selection is user-paced and may remain open for more than 20s.
    // Encoding still has a timeout once the first frame reaches startWriter().
    if (!m_target.isPortal) m_startTimeout.start();
    m_regionThrottle.invalidate();
    m_regionSession.setScreenCapture(nullptr);
    m_session.setScreenCapture(nullptr);
    m_session.setWindowCapture(nullptr);
    m_session.setVideoFrameInput(nullptr);
    m_session.setVideoSink(m_target.isRegion ? nullptr : m_sink.data());
    if (m_target.isWindow) {
        if (!m_target.window.isValid()) { fail(QStringLiteral("선택한 창이 더 이상 존재하지 않습니다.")); return; }
        m_session.setScreenCapture(nullptr);
        m_session.setWindowCapture(&m_window);
        m_window.setWindow(m_target.window);
        m_window.start();
    } else {
        if (!m_target.isPortal && !m_target.screen) { fail(QStringLiteral("선택한 디스플레이가 연결되어 있지 않습니다.")); return; }
        m_session.setWindowCapture(nullptr);
        if (m_target.isRegion) {
            m_regionSession.setScreenCapture(&m_screen);
            m_session.setVideoFrameInput(&m_regionInput);
            m_regionGeometryConnection = connect(m_target.screen, &QScreen::geometryChanged, this, [this] {
                fail(QStringLiteral("디스플레이 구성이 변경되었습니다. 영역을 다시 지정하세요."));
            });
        } else m_session.setScreenCapture(&m_screen);
        if (!m_target.isPortal) m_screen.setScreen(m_target.screen);
        m_screen.start();
    }
}

void RecorderController::handleRegionFrame(const QVideoFrame &frame) {
    if (!m_target.isRegion || !frame.isValid() || m_state == State::Idle || m_state == State::Stopping) return;
    if (!m_target.screen || m_target.screen->size() != m_target.regionScreenSize) {
        fail(QStringLiteral("디스플레이 구성이 변경되었습니다. 녹화를 중지합니다. 영역을 다시 지정하세요."));
        return;
    }
    if (m_regionThrottle.isValid() && m_regionThrottle.elapsed() < 1000 / m_options.fps) return;
    m_regionThrottle.start();
    const auto image = cropRegionImage(frame.toImage(), m_target.region, m_target.regionScreenSize);
    if (image.isNull()) { fail(QStringLiteral("선택한 영역의 캡처 프레임을 변환하지 못했습니다.")); return; }
    QVideoFrame cropped(image);
    if (m_sink) m_sink->setVideoFrame(cropped);
    handleFrame(cropped);
    if (m_writerStarted && m_recorder.recorderState() == QMediaRecorder::RecordingState) {
        // Custom frame inputs retain the timestamps supplied by the application.
        // Exclude pauses explicitly, but preserve gaps caused by dropped busy frames.
        const qint64 time = (m_regionClock.nsecsElapsed() - m_regionPausedDuration) / 1000;
        cropped.setStartTime(time);
        cropped.setEndTime(time + 1000000 / m_options.fps);
        m_regionInput.sendVideoFrame(cropped); // Backpressure: drop, never queue unbounded screen images.
    }
}

void RecorderController::handleFrame(const QVideoFrame &frame) {
    if (!frame.isValid()) return;
    m_sourceSize = frame.size();
    if (m_state != State::Starting || m_writerStarted) return;
    m_startTimeout.stop();
    if (m_outputPath.isEmpty()) {
        setState(State::Preview);
        emit statusChanged(QStringLiteral("미리보기 중 · 파일은 저장되지 않습니다."));
    } else startWriter();
}

void RecorderController::startWriter() {
    if (m_writerStarted || m_state != State::Starting) return;
    m_startTimeout.stop();
    const auto [width, height] = m_options.fittedSize(m_sourceSize.width(), m_sourceSize.height());
    if (width == 0) { fail(QStringLiteral("캡처 프레임의 크기를 확인할 수 없습니다.")); return; }
    if (!recordingFormat(m_codec, m_options.microphone).isSupported(QMediaFormat::Encode)) {
        fail(QStringLiteral("이 환경에서 선택한 MP4 코덱을 사용할 수 없습니다. 다른 코덱을 선택하세요."));
        return;
    }
    const QFileInfo destination(m_outputPath);
    m_temporary = std::make_unique<QTemporaryFile>(destination.dir().filePath(QStringLiteral(".clipdock-XXXXXX.mp4")));
    if (!m_temporary->open()) { fail(QStringLiteral("저장 폴더에 파일을 만들 수 없습니다.")); return; }
    m_temporary->close();
    if (m_options.microphone) {
        m_audio.setDevice(m_audioDevice);
        m_session.setAudioInput(&m_audio);
    } else m_session.setAudioInput(nullptr);
    configureRecorder(m_recorder, m_options, QSize(width, height), m_codec);
    m_recorder.setOutputLocation(QUrl::fromLocalFile(m_temporary->fileName()));
    m_writerStarted = true;
    m_regionClock.start();
    m_regionPausedAt = -1;
    m_regionPausedDuration = 0;
    m_startTimeout.start();
    m_recorder.record();
}

void RecorderController::handleRecorderState(QMediaRecorder::RecorderState state) {
    if (state == QMediaRecorder::RecordingState && m_state != State::Stopping) {
        if (m_target.isRegion && m_regionPausedAt >= 0) {
            m_regionPausedDuration += m_regionClock.nsecsElapsed() - m_regionPausedAt;
            m_regionPausedAt = -1;
        }
        m_startTimeout.stop();
        setState(State::Recording);
        emit statusChanged(QStringLiteral("녹화 중 · %1 FPS · %2 kbps").arg(m_options.fps).arg(m_options.bitrateKbps));
    } else if (state == QMediaRecorder::PausedState && m_state != State::Stopping) {
        if (m_target.isRegion && m_regionPausedAt < 0) m_regionPausedAt = m_regionClock.nsecsElapsed();
        setState(State::Paused);
        emit statusChanged(QStringLiteral("일시정지 중"));
    } else if (state == QMediaRecorder::StoppedState && m_writerStarted) {
        finish();
    }
}

void RecorderController::togglePause() {
    if (m_state == State::Recording) m_recorder.pause();
    else if (m_state == State::Paused) m_recorder.record();
}

void RecorderController::stopCapture() {
    m_startTimeout.stop();
    disconnect(m_regionGeometryConnection);
    m_screen.stop();
    m_window.stop();
    m_regionSession.setScreenCapture(nullptr);
    m_session.setVideoFrameInput(nullptr);
    m_session.setAudioInput(nullptr);
}

void RecorderController::stop() {
    if (m_state == State::Idle || m_state == State::Stopping) return;
    ++m_generation;
    setState(State::Stopping);
    emit statusChanged(QStringLiteral("녹화 파일 마무리 중…"));
    m_startTimeout.stop();
    if (m_writerStarted && m_recorder.recorderState() != QMediaRecorder::StoppedState) {
        // Keep the sources alive until the encoder confirms that MP4 finalization is done.
        m_recorder.stop();
    } else finish();
}

void RecorderController::finish() {
    setState(State::Stopping);
    const bool recorded = m_writerStarted;
    m_writerStarted = false;
    stopCapture();
    QString completionStatus;
    if (recorded && m_temporary) {
        const auto temporaryPath = m_temporary->fileName();
        if (QFileInfo(temporaryPath).size() <= 0) {
            m_failed = true;
            emit errorOccurred(QStringLiteral("녹화 데이터가 없습니다."));
        } else if (m_recorderFailed || m_recorder.duration() <= 0) {
            // An encoder failure may leave recoverable data. Never report it as a completed movie.
            m_temporary->setAutoRemove(false);
            m_failed = true;
            emit errorOccurred(QStringLiteral("녹화 파일이 완성되지 않았을 수 있습니다. 복구용 임시 파일 보관 위치:\n%1").arg(temporaryPath));
        } else if (!m_temporary->rename(m_outputPath)) {
            // Preserve a completed movie if the final destination changed or already exists.
            m_temporary->setAutoRemove(false);
            m_failed = true;
            emit errorOccurred(QStringLiteral("완성된 파일을 이동하지 못했습니다. 임시 파일 보관 위치:\n%1").arg(temporaryPath));
        } else {
            // QTemporaryFile retains a native handle even after close(). Its own
            // rename releases that handle on Windows; keep the completed movie
            // when the temporary-file owner is destroyed below.
            m_temporary->setAutoRemove(false);
            emit saved(m_outputPath);
            completionStatus = (m_failed ? QStringLiteral("캡처 중단 · 중단 전 녹화 저장 완료 · %1")
                                         : QStringLiteral("저장 완료 · %1")).arg(QFileInfo(m_outputPath).fileName());
        }
    }
    if (!completionStatus.isEmpty()) emit statusChanged(completionStatus);
    else if (m_failed) emit statusChanged(QStringLiteral("작업 실패 · 오류 내용을 확인하세요."));
    else if (!recorded) emit statusChanged(QStringLiteral("미리보기 종료"));
    m_temporary.reset();
    m_outputPath.clear();
    setState(State::Idle);
}

void RecorderController::fail(const QString &message, bool recorderError) {
    if (m_state == State::Idle) return;
    m_recorderFailed = m_recorderFailed || recorderError;
    if (m_failed) return;
    m_failed = true;
    emit errorOccurred(message);
    if (m_state != State::Stopping) stop();
}
