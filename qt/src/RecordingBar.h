#pragma once

#include <QWidget>
#include <optional>

class QLabel;
class QPushButton;

class RecordingBar final : public QWidget {
    Q_OBJECT
public:
    RecordingBar();
    void setRecordingState(bool paused, bool microphone);
    void setDuration(const QString &duration);
    bool place(QScreen *preferredScreen, const QRect &captureToAvoid);

signals:
    void pauseRequested();
    void stopRequested();
    void restoreRequested();
    void dismissed();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QLabel *m_status;
    QLabel *m_time;
    QLabel *m_microphone;
    QPushButton *m_pause;
    QRect m_captureToAvoid;
    QPoint m_dragOffset;
    std::optional<QPoint> m_position;
};
