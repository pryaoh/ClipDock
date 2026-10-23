#include "MainWindow.h"
#include "PlatformPermissions.h"
#include "WindowPicker.h"
#include "RegionPicker.h"
#include "UiTheme.h"
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <utility>

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("ClipDock"));
    resize(1120, 780);
    setMinimumSize(920, 620);
    UiTheme::applyLightTheme(this);
    using Icon = UiTheme::Icon;

    auto label = [](const QString &text, const char *name) {
        auto *item = new QLabel(text);
        item->setObjectName(QLatin1String(name));
        return item;
    };
    auto separator = [] {
        auto *line = new QFrame;
        line->setObjectName(QStringLiteral("separator"));
        line->setFixedHeight(1);
        return line;
    };
    auto button = [](const QString &text, Icon kind, const char *name = "") {
        auto *item = new QPushButton(UiTheme::icon(kind), text);
        item->setObjectName(QLatin1String(name));
        item->setIconSize(QSize(16, 16));
        item->setCursor(Qt::PointingHandCursor);
        return item;
    };
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("appSurface"));
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(18);
    auto *header = new QHBoxLayout;
    header->setSpacing(14);
    auto *brand = label({}, "brandIcon");
    brand->setFixedSize(48, 48);
    brand->setAlignment(Qt::AlignCenter);
    brand->setPixmap(UiTheme::icon(Icon::Screen, Qt::white).pixmap(QSize(28, 28), devicePixelRatioF()));
    header->addWidget(brand);
    auto *heading = new QVBoxLayout;
    heading->setSpacing(2);
    heading->addWidget(label(QStringLiteral("ClipDock"), "appTitle"));
    heading->addWidget(label(QStringLiteral("화면의 순간을, 간편하게 기록하세요."), "subtitle"));
    header->addLayout(heading);
    header->addStretch();
    m_stateBadge = label(QStringLiteral("●  대기 중"), "stateBadge");
    m_stateBadge->setAccessibleDescription(QStringLiteral("녹화 상태"));
    header->addWidget(m_stateBadge, 0, Qt::AlignVCenter);
    layout->addLayout(header);

    auto *body = new QHBoxLayout;
    body->setSpacing(18);
    auto *previewCard = new QFrame;
    previewCard->setObjectName(QStringLiteral("previewCard"));
    auto *previewLayout = new QVBoxLayout(previewCard);
    previewLayout->setContentsMargins(20, 18, 20, 16);
    previewLayout->setSpacing(16);
    auto *previewHeader = new QHBoxLayout;
    previewHeader->addWidget(label(QStringLiteral("미리보기"), "cardTitle"));
    previewHeader->addStretch();
    m_preview = button(QStringLiteral("미리보기 시작"), Icon::Play, "previewButton");
    m_preview->setIcon(UiTheme::icon(Icon::Play, QColor(QStringLiteral("#2563eb"))));
    previewHeader->addWidget(m_preview);
    previewLayout->addLayout(previewHeader);

    m_previewStack = new QStackedWidget;
    m_previewStack->setObjectName(QStringLiteral("previewStack"));
    m_previewStack->setMinimumSize(360, 220);
    auto *empty = new QWidget;
    empty->setObjectName(QStringLiteral("emptyPreview"));
    auto *emptyLayout = new QVBoxLayout(empty);
    emptyLayout->setContentsMargins(20, 24, 20, 24);
    emptyLayout->setSpacing(14);
    emptyLayout->addStretch();
    auto *emptyIcon = label({}, "emptyIcon");
    emptyIcon->setFixedSize(88, 88);
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyIcon->setPixmap(UiTheme::icon(Icon::Screen, QColor(QStringLiteral("#6086db"))).pixmap(QSize(48, 48), devicePixelRatioF()));
    emptyLayout->addWidget(emptyIcon, 0, Qt::AlignHCenter);
    m_emptyTitle = label(QStringLiteral("어떤 화면을 담아볼까요?"), "emptyTitle");
    m_emptyTitle->setAlignment(Qt::AlignCenter);
    m_emptyTitle->setWordWrap(true);
    emptyLayout->addWidget(m_emptyTitle);
    m_emptyHint = label(QStringLiteral("오른쪽에서 녹화 대상을 선택한 뒤\n미리보기 또는 녹화를 시작하세요."), "emptyHint");
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setWordWrap(true);
    emptyLayout->addWidget(m_emptyHint);
    emptyLayout->addStretch();
    m_previewStack->addWidget(empty);
    m_video = new QVideoWidget;
    m_video->setAspectRatioMode(Qt::KeepAspectRatio);
    m_previewStack->addWidget(m_video);
    previewLayout->addWidget(m_previewStack, 1);
    previewLayout->addWidget(label(QStringLiteral("원본 비율을 유지하여 미리보기와 영상을 저장합니다."), "previewCaption"));
    m_controller.setVideoSink(m_video->videoSink());
    body->addWidget(previewCard, 1);

    auto *settingsCard = new QFrame;
    settingsCard->setObjectName(QStringLiteral("settingsCard"));
    settingsCard->setFixedWidth(300);
    auto *settingsLayout = new QVBoxLayout(settingsCard);
    settingsLayout->setContentsMargins(18, 18, 14, 16);
    settingsLayout->setSpacing(14);
    settingsLayout->addWidget(label(QStringLiteral("녹화 설정"), "cardTitle"));
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *settings = new QWidget;
    m_settings = settings;
    auto *form = new QVBoxLayout(settings);
    form->setContentsMargins(0, 0, 4, 0);
    form->setSpacing(8);
    auto addField = [&](const QString &text, QWidget *widget) {
        auto *field = new QVBoxLayout;
        field->setSpacing(4);
        auto *caption = label(text, "fieldLabel");
        caption->setBuddy(widget);
        widget->setAccessibleName(text);
        field->addWidget(caption);
        field->addWidget(widget);
        form->addLayout(field);
    };
    m_sourceType = new QComboBox;
    m_sourceType->addItem(QStringLiteral("화면"));
    const bool wayland = QGuiApplication::platformName().startsWith(QStringLiteral("wayland"));
    m_portalCapture = wayland;
    if (!wayland) m_sourceType->addItem(QStringLiteral("창"));
    if (!wayland) m_sourceType->addItem(QStringLiteral("영역"));
    addField(QStringLiteral("녹화 대상"), m_sourceType);
    m_target = new QComboBox;
    m_target->setMinimumContentsLength(12);
    m_target->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_target->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_target->setAccessibleName(QStringLiteral("녹화할 화면 또는 창"));
    form->addWidget(m_target);
    auto *sourceActions = new QHBoxLayout;
    sourceActions->setSpacing(6);
    auto *refresh = button(QStringLiteral("새로고침"), Icon::Refresh);
    m_pickWindow = button(QStringLiteral("클릭으로 선택"), Icon::Window, "pickWindowButton");
    m_pickWindow->setToolTip(WindowPicker::isSupported()
        ? QStringLiteral("화면에 보이는 창을 클릭하여 녹화 대상으로 선택합니다. Esc 또는 오른쪽 클릭으로 취소합니다.")
        : QStringLiteral("이 환경에서는 목록 또는 운영체제의 화면 공유 선택 창을 사용하세요."));
    m_pickRegion = button(QStringLiteral("영역 지정"), Icon::Region, "pickRegionButton");
    m_pickRegion->setToolTip(QStringLiteral("원하는 모니터에서 녹화할 사각형을 드래그하세요. Esc 또는 오른쪽 클릭으로 취소합니다."));
    sourceActions->addWidget(m_pickWindow, 1);
    sourceActions->addWidget(m_pickRegion, 1);
    sourceActions->addWidget(refresh, 1);
    form->addLayout(sourceActions);
    m_regionLabel = label({}, "regionHint");
    m_regionLabel->setWordWrap(true);
    form->addWidget(m_regionLabel);
    form->addWidget(separator());
    m_resolution = new QComboBox;
    for (int edge : {1280, 1920, 2560, 4096})
        m_resolution->addItem(QStringLiteral("긴 변 최대 %1 px").arg(edge), edge);
    m_resolution->setCurrentIndex(1);
    addField(QStringLiteral("출력 크기"), m_resolution);
    m_fps = new QComboBox;
    for (int fps : {10, 15, 25, 30, 60}) m_fps->addItem(QStringLiteral("%1 FPS").arg(fps), fps);
    m_fps->setCurrentIndex(2);
    m_bitrate = new QSpinBox;
    m_bitrate->setRange(100, 100000);
    m_bitrate->setSingleStep(500);
    m_bitrate->setValue(5000);
    m_bitrate->setSuffix(QStringLiteral(" kbps"));
    auto *quality = new QHBoxLayout;
    quality->setSpacing(10);
    for (auto entry : {std::pair<QString, QWidget *>(QStringLiteral("프레임 속도"), m_fps),
                       std::pair<QString, QWidget *>(QStringLiteral("비트레이트"), m_bitrate)}) {
        auto *field = new QVBoxLayout;
        field->setSpacing(4);
        auto *caption = label(entry.first, "fieldLabel");
        caption->setBuddy(entry.second);
        entry.second->setAccessibleName(entry.first);
        field->addWidget(caption);
        field->addWidget(entry.second);
        quality->addLayout(field, 1);
    }
    form->addLayout(quality);
    m_codec = new QComboBox;
    m_codec->addItem(QStringLiteral("H.264 · MP4"), static_cast<int>(QMediaFormat::VideoCodec::H264));
    m_codec->addItem(QStringLiteral("MPEG-4 · MP4 (호환 모드)"), static_cast<int>(QMediaFormat::VideoCodec::MPEG4));
    addField(QStringLiteral("비디오 코덱"), m_codec);
    form->addWidget(separator());
    m_audio = new QComboBox;
    m_audio->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    addField(QStringLiteral("오디오 입력"), m_audio);
    auto *hint = label(wayland
        ? QStringLiteral("Wayland에서는 시작 시 시스템 공유 창에서 대상을 선택합니다. 화면 일부를 자르려면 X11 세션을 사용하세요.")
        : QStringLiteral("시스템 소리를 담으려면 별도의 루프백 입력 장치를 선택하세요."), "hint");
    hint->setWordWrap(true);
    form->addWidget(hint);
    form->addStretch();
    scroll->setWidget(settings);
    settingsLayout->addWidget(scroll, 1);
    body->addWidget(settingsCard);
    layout->addLayout(body, 1);

    auto *controls = new QFrame;
    controls->setObjectName(QStringLiteral("controlBar"));
    auto *controlLayout = new QVBoxLayout(controls);
    controlLayout->setContentsMargins(20, 14, 20, 12);
    controlLayout->setSpacing(10);
    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    auto *timerLayout = new QVBoxLayout;
    timerLayout->setSpacing(0);
    timerLayout->addWidget(label(QStringLiteral("녹화 시간"), "fieldLabel"));
    m_clock = label(QStringLiteral("00:00:00"), "timer");
    m_clock->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_clock->setAccessibleDescription(QStringLiteral("녹화 시간"));
    timerLayout->addWidget(m_clock);
    buttons->addLayout(timerLayout);
    buttons->addStretch();
    m_record = button(QStringLiteral("녹화 시작"), Icon::Record, "recordButton");
    m_record->setIcon(UiTheme::icon(Icon::Record, Qt::white));
    m_pause = button(QStringLiteral("일시정지"), Icon::Pause);
    m_stop = button(QStringLiteral("중지 및 저장"), Icon::Stop);
    m_reveal = button(QStringLiteral("저장 폴더"), Icon::Folder);
    m_reveal->setEnabled(false);
    for (auto *item : {m_record, m_pause, m_stop}) buttons->addWidget(item);
    buttons->addSpacing(6);
    buttons->addWidget(m_reveal);
    controlLayout->addLayout(buttons);
    controlLayout->addWidget(separator());
    m_status = label(QStringLiteral("녹화 대상을 선택하세요."), "statusText");
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    controlLayout->addWidget(m_status);
    layout->addWidget(controls);
    setCentralWidget(central);
    const QList<QWidget *> focusOrder = {m_sourceType, m_target, m_pickWindow, m_pickRegion, refresh,
        m_resolution, m_fps, m_bitrate, m_codec, m_audio, m_preview, m_record, m_pause, m_stop, m_reveal};
    for (qsizetype i = 1; i < focusOrder.size(); ++i)
        QWidget::setTabOrder(focusOrder[i - 1], focusOrder[i]);

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
                           "Linux X11: 화면·창 목록 또는 영역 지정을 사용하세요.\n"
                           "Linux Wayland: 시작 시 시스템 공유 창에서 대상을 선택하고 공유를 허용하세요.\n"
                           "소리는 선택한 입력 장치에서 녹음합니다. 시스템 소리 직접 캡처는 포함하지 않습니다.\n"
                           "창 크기를 녹화 도중 변경하면 처음 정한 출력 해상도로 저장됩니다."));
    });

    connect(refresh, &QPushButton::clicked, this, [this] { refreshTargets(); refreshAudio(); });
    connect(m_pickWindow, &QPushButton::clicked, this, &MainWindow::pickWindow);
    connect(m_pickRegion, &QPushButton::clicked, this, &MainWindow::pickRegion);
    connect(m_sourceType, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTargets);
    connect(m_target, &QComboBox::currentIndexChanged, this, [this] {
        m_target->setToolTip(m_target->currentText());
        updateRegionLabel(); updateState(m_controller.state());
    });
    connect(m_audio, &QComboBox::currentIndexChanged, this, [this] {
        m_audio->setToolTip(m_audio->currentText());
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
    m_target->setEnabled(!m_portalCapture);
    if (m_portalCapture) {
        RecorderController::Target target;
        target.isPortal = true;
        m_targets.append(target);
        m_target->addItem(QStringLiteral("시작 시 시스템 공유 창에서 선택"));
    } else if (m_sourceType->currentIndex() != 1) {
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
            const auto name = screen->name().isEmpty() ? QStringLiteral("디스플레이 %1").arg(m_targets.size()) : screen->name();
            m_target->addItem(QStringLiteral("%1 · %2 × %3").arg(name)
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
        m_regionLabel->setText(QStringLiteral("'영역 지정'을 누르고 원하는 모니터에서 드래그하세요. 영역 안에 보이는 모든 내용이 녹화됩니다."));
        m_pickRegion->setText(QStringLiteral("영역 지정"));
        return;
    }
    const auto r = m_targets[index].region;
    m_regionLabel->setText(QStringLiteral("선택 영역: %1 × %2 · 위치 (%3, %4) · 화면 좌표 기준")
        .arg(r.width()).arg(r.height()).arg(r.x()).arg(r.y()));
    m_pickRegion->setText(QStringLiteral("영역 다시 지정"));
}

void MainWindow::pickRegion() {
    if (m_regionPicker || m_controller.state() != RecorderController::State::Idle
        || m_sourceType->currentIndex() != 2) return;
    // A display may have been connected since the target list was last shown.
    refreshTargets();
    const int index = m_target->currentIndex();
    if (index < 0 || index >= m_targets.size()) return;
    const QPointer<QScreen> screen = m_targets[index].screen;
    if (!screen) { refreshTargets(); return; }
    m_regionPicker = new RegionSelection(this);
    connect(m_regionPicker, &RegionSelection::finished, this, [this](QScreen *selectedScreen, const QRect &region) {
        if (!region.isEmpty() && selectedScreen) {
            for (qsizetype i = 0; i < m_targets.size(); ++i) {
                auto &target = m_targets[i];
                if (target.isRegion && target.screen == selectedScreen) {
                    target.region = region;
                    target.regionScreenSize = selectedScreen->size();
                    m_target->setCurrentIndex(int(i));
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
    m_regionPicker->start(screen);
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
    m_pause->setIcon(UiTheme::icon(state == State::Paused ? UiTheme::Icon::Play : UiTheme::Icon::Pause));
    m_stop->setEnabled(!idle && state != State::Stopping);
    m_stop->setText(preview ? QStringLiteral("미리보기 종료") : QStringLiteral("중지 및 저장"));

    QString badge, tone;
    switch (state) {
    case State::Idle: badge = QStringLiteral("대기 중"); break;
    case State::WaitingForPermission: badge = QStringLiteral("권한 확인 중"); tone = QStringLiteral("active"); break;
    case State::Starting: badge = QStringLiteral("준비 중"); tone = QStringLiteral("active"); break;
    case State::Preview: badge = QStringLiteral("미리보기 중"); tone = QStringLiteral("active"); break;
    case State::Recording: badge = QStringLiteral("녹화 중"); tone = QStringLiteral("recording"); break;
    case State::Paused: badge = QStringLiteral("일시정지"); tone = QStringLiteral("paused"); break;
    case State::Stopping: badge = QStringLiteral("마무리 중"); tone = QStringLiteral("active"); break;
    }
    m_stateBadge->setText(QStringLiteral("●  %1").arg(badge));
    if (m_stateBadge->property("tone").toString() != tone) {
        m_stateBadge->setProperty("tone", tone);
        m_stateBadge->style()->unpolish(m_stateBadge);
        m_stateBadge->style()->polish(m_stateBadge);
    }
    const bool showVideo = preview || state == State::Recording || state == State::Paused || state == State::Stopping;
    m_previewStack->setCurrentIndex(showVideo ? 1 : 0);
    if (state == State::WaitingForPermission) {
        m_emptyTitle->setText(QStringLiteral("화면 공유를 기다리고 있어요"));
        m_emptyHint->setText(QStringLiteral("권한 요청이나 시스템 공유 창을 확인하세요.\n중지 버튼으로 취소할 수 있습니다."));
    } else if (state == State::Starting) {
        m_emptyTitle->setText(QStringLiteral("화면을 준비하고 있어요"));
        m_emptyHint->setText(QStringLiteral("선택한 화면을 불러오면 여기에 표시됩니다."));
    } else if (idle) {
        m_emptyTitle->setText(QStringLiteral("어떤 화면을 담아볼까요?"));
        m_emptyHint->setText(QStringLiteral("오른쪽에서 녹화 대상을 선택한 뒤\n미리보기 또는 녹화를 시작하세요."));
    }
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
