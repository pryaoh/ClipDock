#pragma once

#include <QCapturableWindow>
#include <QObject>
#include <QTimer>
#include <QList>
#include "NativeWindowPicker.h"

class QWidget;

// Temporarily covers each display so the selection click never reaches the target app.
class WindowPicker final : public QObject {
    Q_OBJECT
public:
    explicit WindowPicker(QObject *parent = nullptr);
    ~WindowPicker() override;
    static bool isSupported();
    void start();

signals:
    // A null window means cancellation. The receiver restores its UI before overlays close.
    void finished(const QCapturableWindow &window);

private:
    void refreshWindows();
    void updateHover(const QPointF &globalPosition);
    void press(const QPointF &globalPosition);
    void release(const QPointF &globalPosition);
    void finish(const QCapturableWindow &window = {});
    QList<QWidget *> m_overlays;
    QTimer m_hoverTimer;
    QTimer m_timeout;
    QList<NativeWindowCandidate> m_windows;
    QList<QCapturableWindow> m_captureWindows;
    NativeWindowCandidate m_hover;
    QCapturableWindow m_hoverWindow;
    quint64 m_pressedId = 0;
    bool m_finished = false;
};
