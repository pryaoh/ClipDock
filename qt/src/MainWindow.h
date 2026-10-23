#pragma once
#include "RecorderController.h"
#include <QMainWindow>
#include <QMediaDevices>
#include <QList>

class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QVideoWidget;
class QStackedWidget;
class QCloseEvent;
class QWidget;
class WindowPicker;
class RegionPicker;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    friend class MainWindowTestPeer;
    void refreshTargets();
    void pickWindow();
    void pickRegion();
    void updateRegionLabel();
    void refreshAudio();
    void begin(bool recording);
    void updateState(RecorderController::State state);
    void showError(const QString &message);

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
    QPushButton *m_record = nullptr;
    QPushButton *m_preview = nullptr;
    QPushButton *m_pickWindow = nullptr;
    QPointer<WindowPicker> m_picker;
    QPointer<RegionPicker> m_regionPicker;
    QPushButton *m_pickRegion = nullptr;
    QLabel *m_regionLabel = nullptr;
    QPushButton *m_pause = nullptr;
    QPushButton *m_stop = nullptr;
    QPushButton *m_reveal = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_clock = nullptr;
    QLabel *m_stateBadge = nullptr;
    QLabel *m_emptyTitle = nullptr;
    QLabel *m_emptyHint = nullptr;
    QStackedWidget *m_previewStack = nullptr;
    QVideoWidget *m_video = nullptr;
    QString m_lastFile;
    bool m_closing = false;
    bool m_portalCapture = false;
};
