#include "MainWindow.h"
#include "PlatformPermissions.h"
#include "WindowPicker.h"
#include "RegionPicker.h"
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QVideoWidget>

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("ClipDock · Qt 6"));
    resize(820, 730);
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 24, 24, 20);
    layout->setSpacing(16);
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("<h2>ClipDock</h2>화면과 소리를 동영상으로 기록하세요."));
    header->addWidget(title);
    m_clock = new QLabel(QStringLiteral("00:00:00"));
    m_clock->setStyleSheet(QStringLiteral("font-size: 24px; font-family: monospace;"));
    header->addStretch();
    header->addWidget(m_clock);
    layout->addLayout(header);

    auto *group = new QGroupBox(QStringLiteral("녹화 설정"));
    m_settings = group;
    auto *form = new QFormLayout(group);
    form->setVerticalSpacing(12);
    auto *sources = new QHBoxLayout;
    m_sourceType = new QComboBox;
    m_sourceType->addItem(QStringLiteral("화면"));
    const bool wayland = QGuiApplication::platformName().startsWith(QStringLiteral("wayland"));
    if (!wayland) m_sourceType->addItem(QStringLiteral("창"));
    if (!wayland) m_sourceType->addItem(QStringLiteral("영역"));
    m_target = new QComboBox;
    m_target->setMinimumContentsLength(20);
    m_target->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    auto *refresh = new QPushButton(QStringLiteral("새로고침"));
    m_pickWindow = new QPushButton(QStringLiteral("클릭으로 선택"));
    m_pickWindow->setObjectName(QStringLiteral("pickWindowButton"));
    m_pickWindow->setToolTip(WindowPicker::isSupported()
        ? QStringLiteral("화면에 보이는 창을 클릭하여 녹화 대상으로 선택합니다. Esc 또는 오른쪽 클릭으로 취소합니다.")
        : QStringLiteral("이 환경에서는 목록 또는 운영체제의 화면 공유 선택 창을 사용하세요."));
    sources->addWidget(m_sourceType);
    sources->addWidget(m_target, 1);
    sources->addWidget(m_pickWindow);
    m_pickRegion = new QPushButton(QStringLiteral("영역 지정"));
    m_pickRegion->setObjectName(QStringLiteral("pickRegionButton"));
    m_pickRegion->setToolTip(QStringLiteral("선택한 모니터에서 녹화할 사각형을 드래그하세요. Esc 또는 오른쪽 클릭으로 취소합니다."));
    sources->addWidget(m_pickRegion);
    sources->addWidget(refresh);
    form->addRow(QStringLiteral("녹화 대상"), sources);
    m_regionLabel = new QLabel;
    m_regionLabel->setWordWrap(true);
    form->addRow(m_regionLabel);
    auto *quality = new QHBoxLayout;
    m_resolution = new QComboBox;
    for (int edge : {1280, 1920, 2560, 4096})
        m_resolution->addItem(QStringLiteral("긴 변 최대 %1 px").arg(edge), edge);
    m_resolution->setCurrentIndex(1);
    m_fps = new QComboBox;
    for (int fps : {10, 15, 25, 30, 60}) m_fps->addItem(QStringLiteral("%1 FPS").arg(fps), fps);
    m_fps->setCurrentIndex(2);
    quality->addWidget(m_resolution, 1);
    quality->addWidget(m_fps);
    form->addRow(QStringLiteral("해상도 / 속도"), quality);
    m_bitrate = new QSpinBox;
    m_bitrate->setRange(100, 100000);
    m_bitrate->setSingleStep(500);
    m_bitrate->setValue(5000);
    m_bitrate->setSuffix(QStringLiteral(" kbps"));
    form->addRow(QStringLiteral("비트레이트"), m_bitrate);
    m_codec = new QComboBox;
    m_codec->addItem(QStringLiteral("H.264 · MP4"), static_cast<int>(QMediaFormat::VideoCodec::H264));
    m_codec->addItem(QStringLiteral("MPEG-4 · MP4 (호환 모드)"), static_cast<int>(QMediaFormat::VideoCodec::MPEG4));
    form->addRow(QStringLiteral("비디오 코덱"), m_codec);
    m_audio = new QComboBox;
    form->addRow(QStringLiteral("오디오 입력"), m_audio);
    auto *hint = new QLabel(wayland
        ? QStringLiteral("Wayland에서는 시작 시 OS가 화면/창 선택 창을 표시합니다.")
        : QStringLiteral("원본 화면 비율을 유지합니다. 시스템 소리는 별도 루프백 입력 장치가 필요합니다."));
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #707070;"));
    form->addRow(hint);
    layout->addWidget(group);

    m_video = new QVideoWidget;
    m_video->setMinimumSize(480, 240);
    m_video->setAspectRatioMode(Qt::KeepAspectRatio);
    m_video->setStyleSheet(QStringLiteral("background: #15171a;"));
    layout->addWidget(m_video, 1);
    m_controller.setVideoSink(m_video->videoSink());

    auto *buttons = new QHBoxLayout;
    m_record = new QPushButton(QStringLiteral("● 녹화 시작"));
    m_record->setStyleSheet(QStringLiteral("QPushButton:enabled { background: #c52e39; color: white; border-radius: 5px; padding: 8px 15px; }"));
    m_preview = new QPushButton(QStringLiteral("미리보기"));
    m_pause = new QPushButton(QStringLiteral("일시정지"));
    m_stop = new QPushButton(QStringLiteral("중지 및 저장"));
    for (auto *button : {m_record, m_preview, m_pause, m_stop}) buttons->addWidget(button);
    buttons->addStretch();
    m_reveal = new QPushButton(QStringLiteral("저장 폴더"));
    m_reveal->setEnabled(false);
    buttons->addWidget(m_reveal);
    layout->addLayout(buttons);
    m_status = new QLabel(QStringLiteral("녹화 대상을 선택하세요."));
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_status);
    setCentralWidget(central);

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("파일"));
    auto *quit = fileMenu->addAction(QStringLiteral("종료"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    auto *help = menuBar()->addMenu(QStringLiteral("도움말"));
    auto *permissions = help->addAction(QStringLiteral("권한 설정 열기"));
    connect(permissions, &QAction::triggered, this, &openScreenPermissionSettings);
#ifdef Q_OS_MACOS
    connect(help->addAction(QStringLiteral("권한 상태 확인")), &QAction::triggered, this, [this] {
        QMessageBox dialog(QMessageBox::Information, QStringLiteral("ClipDock 권한 상태"),
                           screenPermissionDiagnostics(), QMessageBox::Ok, this);
        dialog.setTextFormat(Qt::PlainText);
        auto *settings = dialog.addButton(QStringLiteral("권한 설정 열기"), QMessageBox::ActionRole);
        dialog.exec();
        if (dialog.clickedButton() == settings) openScreenPermissionSettings();
    });
#endif
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    permissions->setVisible(false);
#endif
    connect(help->addAction(QStringLiteral("사용 안내")), &QAction::triggered, this, [this] {
        QMessageBox::information(this, QStringLiteral("ClipDock"),
            QStringLiteral("화면/창 선택 → 미리보기 또는 녹화 시작 → 중지 및 저장\n\n"
                           "창 모드의 '클릭으로 선택': 원하는 창을 클릭하세요. Esc/오른쪽 클릭으로 취소합니다.\n\n"
                           "영역 모드: 모니터 선택 → 영역 지정 → 드래그 → 미리보기 또는 녹화 시작.\n"
                           "한 모니터 안에서 16 × 16 이상으로 선택하세요. 영역 안에 겹친 창도 녹화됩니다.\n\n"
                           "단축키 (앱에 포커스가 있을 때):\n"
                           "Ctrl/⌘+Shift+R 녹화 · Ctrl/⌘+Shift+P 일시정지 · Ctrl/⌘+Shift+S 중지\n\n"
                           "macOS: 화면 기록 권한을 허용한 뒤 재실행하세요.\n"
                           "소리는 선택한 입력 장치에서 녹음합니다. 시스템 소리 직접 캡처는 포함하지 않습니다.\n"
                           "창 크기를 녹화 도중 변경하면 처음 정한 출력 해상도로 저장됩니다."));
    });

    connect(refresh, &QPushButton::clicked, this, [this] { refreshTargets(); refreshAudio(); });
    connect(m_pickWindow, &QPushButton::clicked, this, &MainWindow::pickWindow);
    connect(m_pickRegion, &QPushButton::clicked, this, &MainWindow::pickRegion);
    connect(m_sourceType, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTargets);
    connect(m_target, &QComboBox::currentIndexChanged, this, [this] {
        updateRegionLabel(); updateState(m_controller.state());
    });
    connect(&m_devices, &QMediaDevices::audioInputsChanged, this, [this] {
        if (m_controller.state() == RecorderController::State::Idle) refreshAudio();
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this] {
        if (m_controller.state() == RecorderController::State::Idle) refreshTargets();
    });
    connect(m_record, &QPushButton::clicked, this, [this] { begin(true); });
    connect(m_preview, &QPushButton::clicked, this, [this] { begin(false); });
    connect(m_pause, &QPushButton::clicked, &m_controller, &RecorderController::togglePause);
    connect(m_stop, &QPushButton::clicked, &m_controller, &RecorderController::stop);
    connect(m_reveal, &QPushButton::clicked, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_lastFile).absolutePath()));
    });
    connect(&m_controller, &RecorderController::stateChanged, this, &MainWindow::updateState);
    connect(&m_controller, &RecorderController::statusChanged, m_status, &QLabel::setText);
    connect(&m_controller, &RecorderController::errorOccurred, this, &MainWindow::showError, Qt::QueuedConnection);
    connect(&m_controller, &RecorderController::saved, this, [this](const QString &path) {
        m_lastFile = path;
        m_reveal->setEnabled(true);
    });
    connect(&m_controller, &RecorderController::durationChanged, this, [this](qint64 milliseconds) {
        const auto seconds = milliseconds / 1000;
        m_clock->setText(QStringLiteral("%1:%2:%3").arg(seconds / 3600, 2, 10, QLatin1Char('0'))
            .arg(seconds / 60 % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0')));
    });
    auto *recordShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+R")), this);
    connect(recordShortcut, &QShortcut::activated, this, [this] { if (m_record->isEnabled()) begin(true); });
    auto *pauseShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+P")), this);
    connect(pauseShortcut, &QShortcut::activated, &m_controller, &RecorderController::togglePause);
    auto *stopShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")), this);
    connect(stopShortcut, &QShortcut::activated, &m_controller, &RecorderController::stop);
    refreshTargets();
    refreshAudio();
    updateState(RecorderController::State::Idle);
}

void MainWindow::refreshTargets() {
    if (m_controller.state() != RecorderController::State::Idle) return;
    const int oldIndex = m_target->currentIndex();
    const auto previous = oldIndex >= 0 && oldIndex < m_targets.size() ? m_targets[oldIndex] : RecorderController::Target();
    const bool regionMode = m_sourceType->currentIndex() == 2;
    m_pickWindow->setVisible(m_sourceType->currentIndex() == 1);
    m_pickWindow->setEnabled(WindowPicker::isSupported());
    m_pickRegion->setVisible(regionMode);
    m_regionLabel->setVisible(regionMode);
    m_target->clear();
    m_targets.clear();
    if (m_sourceType->currentIndex() != 1) {
        for (auto *screen : QGuiApplication::screens()) {
            RecorderController::Target target;
            target.screen = screen;
            target.isRegion = regionMode;
            if (regionMode && previous.isRegion && previous.screen == screen
                && previous.regionScreenSize == screen->size()) {
                target.region = previous.region;
                target.regionScreenSize = previous.regionScreenSize;
            }
            m_targets.append(target);
            m_target->addItem(QStringLiteral("%1 · %2 × %3").arg(screen->name())
                .arg(screen->size().width()).arg(screen->size().height()));
        }
    } else {
        QString message;
        if (!requestScreenPermission(message)) { showError(message); return; }
        const auto capturedWindows = QWindowCapture::capturableWindows();
        QList<QCapturableWindow> selectableWindows;
        if (WindowPicker::isSupported()) {
            // Use the same visible app windows as the point picker (not Dock/system surfaces).
            for (const auto &candidate : nativePickerWindows()) {
                const auto window = capturableWindowWithId(capturedWindows, candidate.id);
                if (window.isValid()) selectableWindows.append(window);
            }
        } else {
            selectableWindows = capturedWindows;
        }
        for (const auto &window : selectableWindows) {
            if (window.isValid()) {
                RecorderController::Target target;
                target.window = window;
                target.isWindow = true;
                m_targets.append(target);
                m_target->addItem(window.description().isEmpty() ? QStringLiteral("제목 없는 창") : window.description());
            }
        }
    }
    if (!previous.isWindow && previous.screen) {
        for (qsizetype i = 0; i < m_targets.size(); ++i)
            if (m_targets[i].screen == previous.screen) m_target->setCurrentIndex(int(i));
    }
    if (m_targets.isEmpty()) m_status->setText(QStringLiteral("사용 가능한 대상이 없습니다. 권한을 확인하고 새로고침하세요."));
    updateRegionLabel();
    updateState(m_controller.state());
}

void MainWindow::updateRegionLabel() {
    const int index = m_target->currentIndex();
    if (index < 0 || index >= m_targets.size() || m_targets[index].region.isEmpty()) {
        m_regionLabel->setText(QStringLiteral("모니터를 선택하고 '영역 지정'을 누르세요. 영역 안에 보이는 모든 내용이 녹화됩니다."));
        m_pickRegion->setText(QStringLiteral("영역 지정"));
        return;
    }
    const auto r = m_targets[index].region;
    m_regionLabel->setText(QStringLiteral("선택 영역: %1 × %2 · 위치 (%3, %4) · 화면 좌표 기준")
        .arg(r.width()).arg(r.height()).arg(r.x()).arg(r.y()));
    m_pickRegion->setText(QStringLiteral("영역 다시 지정"));
}

void MainWindow::pickRegion() {
    const int index = m_target->currentIndex();
    if (m_regionPicker || m_controller.state() != RecorderController::State::Idle
        || m_sourceType->currentIndex() != 2 || index < 0 || index >= m_targets.size()) return;
    const QPointer<QScreen> screen = m_targets[index].screen;
    if (!screen) { refreshTargets(); return; }
    const QSize screenSize = screen->size();
    m_regionPicker = new RegionPicker(screen);
    connect(m_regionPicker, &RegionPicker::finished, this, [this, screen, screenSize](const QRect &region) {
        if (!region.isEmpty() && screen && screen->size() == screenSize) {
            for (auto &target : m_targets) {
                if (target.isRegion && target.screen == screen) {
                    target.region = region;
                    target.regionScreenSize = screenSize;
                    break;
                }
            }
            m_status->setText(QStringLiteral("영역을 선택했습니다. 미리보기 또는 녹화 시작을 누르세요."));
        } else m_status->setText(QStringLiteral("영역 선택을 취소했습니다. 기존 영역을 유지합니다."));
        updateRegionLabel();
        updateState(m_controller.state());
        show(); raise(); activateWindow();
    });
    hide();
    m_regionPicker->start();
}

void MainWindow::pickWindow() {
    if (m_picker || m_controller.state() != RecorderController::State::Idle
        || m_sourceType->currentIndex() != 1 || !WindowPicker::isSupported()) return;
    QString message;
    if (!requestScreenPermission(message)) { showError(message); return; }
    m_picker = new WindowPicker(this);
    connect(m_picker, &WindowPicker::finished, this, [this](const QCapturableWindow &window) {
        show();
        raise();
        activateWindow();
        if (window.isValid()) {
            // Re-enumerate after selection; another app may have opened or closed a window.
            // Compare window identity, never its title.
            refreshTargets();
            m_target->setCurrentIndex(-1);
            bool found = false;
            for (qsizetype i = 0; i < m_targets.size(); ++i) {
                if (m_targets[i].isWindow && m_targets[i].window == window) {
                    m_target->setCurrentIndex(static_cast<int>(i));
                    m_status->setText(QStringLiteral("선택한 창: %1 · 미리보기 또는 녹화 시작을 누르세요.")
                        .arg(m_target->currentText()));
                    found = true;
                    break;
                }
            }
            if (!found) m_status->setText(QStringLiteral("선택한 창이 닫혔거나 녹화할 수 없습니다. 다시 선택하세요."));
        } else {
            m_status->setText(QStringLiteral("창 선택을 취소했습니다. 기존 녹화 대상을 유지합니다."));
        }
    });
    hide();
    m_picker->start();
}

void MainWindow::refreshAudio() {
    const auto previous = m_audio->currentData().toByteArray();
    m_audio->clear();
    m_audio->addItem(QStringLiteral("녹음 안 함"), QByteArray());
    m_audioDevices = QMediaDevices::audioInputs();
    for (const auto &device : m_audioDevices) m_audio->addItem(device.description(), device.id());
    const auto index = m_audio->findData(previous);
    m_audio->setCurrentIndex(std::max(0, index));
}

void MainWindow::begin(bool recording) {
    const int target = m_target->currentIndex();
    if (target < 0 || target >= m_targets.size()) return;
    // A modal file dialog still processes screen removal and target-list refresh events.
    const auto selectedTarget = m_targets[target];
    QString outputPath;
    if (recording) {
        const auto name = QStringLiteral("ClipDock_%1.mp4").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")));
        const auto folder = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        outputPath = QFileDialog::getSaveFileName(this, QStringLiteral("녹화 파일 저장"),
            QDir(folder).filePath(name), QStringLiteral("MP4 동영상 (*.mp4)"), nullptr, QFileDialog::DontConfirmOverwrite);
        if (outputPath.isEmpty()) return;
        if (!outputPath.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive)) outputPath += QStringLiteral(".mp4");
    }
    RecordingOptions options;
    options.maxEdge = m_resolution->currentData().toInt();
    options.fps = m_fps->currentData().toInt();
    options.bitrateKbps = m_bitrate->value();
    options.microphone = recording && m_audio->currentIndex() > 0;
    const int audioIndex = m_audio->currentIndex() - 1;
    const auto audio = audioIndex >= 0 && audioIndex < m_audioDevices.size() ? m_audioDevices[audioIndex] : QAudioDevice();
    if (!selectedTarget.isAvailable()) {
        showError(QStringLiteral("선택한 녹화 대상이 사라졌거나 변경되었습니다. 대상을 다시 선택하세요."));
        return;
    }
    m_controller.start(selectedTarget, options, audio,
        static_cast<QMediaFormat::VideoCodec>(m_codec->currentData().toInt()), outputPath);
}

void MainWindow::updateState(RecorderController::State state) {
    using State = RecorderController::State;
    const bool idle = state == State::Idle;
    const bool preview = state == State::Preview;
    const int index = m_target->currentIndex();
    const bool hasTarget = index >= 0 && index < m_targets.size()
        && (!m_targets[index].isRegion || !m_targets[index].region.isEmpty());
    m_settings->setEnabled(idle);
    m_pickRegion->setEnabled(idle && index >= 0);
    m_record->setEnabled((idle || preview) && hasTarget);
    m_preview->setEnabled(idle && hasTarget);
    m_pause->setEnabled(state == State::Recording || state == State::Paused);
    m_pause->setText(state == State::Paused ? QStringLiteral("재개") : QStringLiteral("일시정지"));
    m_stop->setEnabled(!idle && state != State::Stopping);
    m_stop->setText(preview ? QStringLiteral("미리보기 종료") : QStringLiteral("중지 및 저장"));
    if (idle && m_closing) QTimer::singleShot(0, this, [this] { if (m_closing) close(); });
}

void MainWindow::showError(const QString &message) {
    m_closing = false;
    QMessageBox::warning(this, QStringLiteral("ClipDock"), message);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_controller.state() == RecorderController::State::Idle) { event->accept(); return; }
    m_closing = true;
    event->ignore();
    m_controller.stop();
}
