#pragma once

#include <QCapturableWindow>
#include <QImage>
#include <QObject>

class QThread;

// Supplies the initial Windows window image that Qt 6.8 can discard while
// reconciling native frame bounds. Subsequent updates remain owned by Qt.
class WindowCaptureBootstrap final : public QObject {
    Q_OBJECT
public:
    explicit WindowCaptureBootstrap(QObject *parent = nullptr);
    ~WindowCaptureBootstrap() override;
    void start(const QCapturableWindow &window);
    void stop();

signals:
    void frameReady(const QImage &image);

private:
    QThread *m_worker = nullptr;
    quint64 m_generation = 0;
};
