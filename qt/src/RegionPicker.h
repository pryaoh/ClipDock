#pragma once
#include <QPointer>
#include <QList>
#include <QRect>
#include <QWidget>

class QScreen;
class QLabel;

// One selected monitor at a time; a drag never spills into a different display.
class RegionPicker final : public QWidget {
    Q_OBJECT
public:
    explicit RegionPicker(QScreen *screen);
    void start();
signals:
    void finished(const QRect &region);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    void updateSelection(const QPointF &point);
    void finish(const QRect &region = {});
    QPointer<QScreen> m_screen;
    QLabel *m_hint;
    QPointF m_anchor;
    QRect m_region;
    bool m_dragging = false;
    bool m_finished = false;
};

// One overlay per display keeps each drag in that display's logical coordinates.
class RegionSelection final : public QObject {
    Q_OBJECT
public:
    explicit RegionSelection(QObject *parent = nullptr);
    ~RegionSelection() override;
    void start(QScreen *preferredScreen);
signals:
    void finished(QScreen *screen, const QRect &region);
private:
    void finish(QScreen *screen = nullptr, const QRect &region = {});
    QList<RegionPicker *> m_overlays;
    bool m_finished = false;
};
