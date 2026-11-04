#pragma once
#include "RecorderController.h"
#include "RecordingPreferences.h"
#include <QMainWindow>
#include <QMediaDevices>
#include <QList>

class QComboBox;
class QCheckBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QVideoWidget;
class QStackedWidget;
class QCloseEvent;
class QWidget;
class WindowPicker;
class RegionSelection;
class GlobalShortcuts;
enum class ShortcutAction;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    friend class MainWindowTestPeer;
    void restorePreferences();
    void savePreferences();
    void resetPreferences();
    void setRegion(QScreen *screen, const QRect &region);
    void watchScreen(QScreen *screen);
    void refreshTargets();
    void pickWindow();
    void pickRegion();
    void updateRegionLabel();
    void refreshAudio();
    void begin(bool recording);
    void updateState(RecorderController::State state);
    void applyCaptureExclusion();
    void updatePreview();
    bool overlapsCapture() const;
    bool needsFallbackMinimize() const;
    void minimizeForRecording();
    void showError(const QString &message);
    void dispatchShortcut(ShortcutAction action, const QString &activationToken = QString());

    RecordingPreferences m_preferences;
    bool m_restoringPreferences = true;
    QPointer<QScreen> m_regionScreen;
    QAction *m_resetPreferences = nullptr;
    RecorderController m_controller;
    QMediaDevices m_devices;
    QList<RecorderController::Target> m_targets;
    QList<QAudioDevice> m_audioDevices;
    QWidget *m_settings = nullptr;
    QComboBox *m_sourceType = nullptr;
    QComboBox *m_target = nullptr;
    QComboBox *m_resolution = nullptr;
    QComboBox *m_fps = nullptr;
    QComboBox *m_audio = nullptr;
    QComboBox *m_codec = nullptr;
    QSpinBox *m_bitrate = nullptr;
    QCheckBox *m_excludeSelf = nullptr;
    QCheckBox *m_minimizeRecording = nullptr;
    QPushButton *m_record = nullptr;
    QPushButton *m_preview = nullptr;
    QPushButton *m_pickWindow = nullptr;
    QPointer<WindowPicker> m_picker;
    QPointer<RegionSelection> m_regionPicker;
    QPushButton *m_pickRegion = nullptr;
    QLabel *m_regionLabel = nullptr;
    QPushButton *m_pause = nullptr;
    QPushButton *m_stop = nullptr;
    QPushButton *m_reveal = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_clock = nullptr;
    GlobalShortcuts *m_shortcuts = nullptr;
    bool m_selectingOutput = false;
    QLabel *m_stateBadge = nullptr;
    QLabel *m_emptyTitle = nullptr;
    QLabel *m_emptyHint = nullptr;
    QLabel *m_captureHint = nullptr;
    QStackedWidget *m_previewStack = nullptr;
    QVideoWidget *m_video = nullptr;
    QString m_lastFile;
    bool m_closing = false;
    bool m_portalCapture = false;
    bool m_captureReady = false;
    bool m_captureExcluded = false;
    bool m_autoMinimized = false;
    bool m_restoreMaximized = false;
    bool m_fallbackPausePending = false;
    RecorderController::State m_captureState = RecorderController::State::Idle;
};
