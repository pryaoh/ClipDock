#include "WindowPicker.h"
#include <QApplication>
#include <QCursor>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QWindowCapture>
#include <functional>
#include <utility>

namespace {
class PickerOverlay final : public QWidget {
public:
    using PointAction = std::function<void(const QPointF &)>;
    PickerOverlay(QScreen *screen, PointAction hover, PointAction press, PointAction release,
                  std::function<void()> cancel)
        : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
          m_scale(pickerNativeScale(screen)), m_hover(std::move(hover)), m_press(std::move(press)),
          m_release(std::move(release)), m_cancel(std::move(cancel)) {
        setAttribute(Qt::WA_TranslucentBackground);
        setWindowTitle(QStringLiteral("녹화할 창 선택"));
        setCursor(Qt::CrossCursor);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setGeometry(screen->geometry());
        m_label = new QLabel(this);
        m_label->setTextFormat(Qt::PlainText);
        m_label->setAlignment(Qt::AlignCenter);
        m_label->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_label->setStyleSheet(QStringLiteral(
            "background: #17212e; color: white; border: 2px solid #58a6ff;"
            "border-radius: 10px; padding: 12px; font-size: 16px;"));
        m_label->setFixedSize(qMin(720, qMax(1, width() - 32)), 96);
        m_label->move((width() - m_label->width()) / 2, 32);
    }
    void showCandidate(const NativeWindowCandidate &candidate, bool capturable, const QPointF &pointer) {
        m_highlight = candidate.id ? pickerOverlayRect(candidate.bounds, geometry(), m_scale) : QRectF();
        m_capturable = capturable;
        QString text;
        if (!candidate.id) text = QStringLiteral("녹화할 앱 창 위로 마우스를 이동하세요.");
        else if (!capturable) text = QStringLiteral("녹화할 수 없는 창입니다. 다른 창을 선택하세요.");
        else text = QStringLiteral("선택할 창: %1").arg(candidate.title.isEmpty()
            ? QStringLiteral("제목 없는 창") : candidate.title);
        const auto title = m_label->fontMetrics().elidedText(text, Qt::ElideRight, m_label->width() - 32);
        m_label->setText(title + QStringLiteral("\n파란 테두리의 창 클릭 · Esc / 오른쪽 클릭: 취소"));
        // Keep the hint away from the pointer instead of obscuring the hovered window's controls.
        const auto localPointer = pointer - QPointF(geometry().topLeft());
        const int hintY = localPointer.y() < 160 ? qMax(0, height() - 128) : 32;
        m_label->move((width() - m_label->width()) / 2, hintY);
        setProperty("highlightRect", m_highlight);
        setProperty("highlightWindowId", QVariant::fromValue(candidate.id));
        update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        // Keep even the highlighted area nonzero-alpha so selection cannot click through.
        painter.fillRect(rect(), QColor(0, 0, 0, 1));
        QPainterPath shade;
        shade.setFillRule(Qt::OddEvenFill);
        shade.addRect(rect());
        const QRectF visible = m_highlight.intersected(QRectF(rect()));
        if (!visible.isEmpty()) shade.addRect(visible);
        painter.fillPath(shade, QColor(0, 0, 0, 48));
        if (!visible.isEmpty()) {
            const QColor color = m_capturable ? QColor(45, 150, 255) : QColor(235, 160, 50);
            painter.fillRect(visible, QColor(color.red(), color.green(), color.blue(), 28));
            QPen pen(color, 3);
            pen.setCosmetic(true);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            // Draw the real window frame; clipping happens per display for spanning windows.
            painter.drawRect(m_highlight.adjusted(2, 2, -2, -2));
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override { m_hover(event->globalPosition()); }
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) m_press(event->globalPosition());
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) m_release(event->globalPosition());
        else if (event->button() == Qt::RightButton) m_cancel();
    }
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) m_cancel();
        else QWidget::keyPressEvent(event);
    }
    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        m_cancel();
    }
private:
    QLabel *m_label;
    QRectF m_highlight;
    bool m_capturable = false;
    qreal m_scale;
    PointAction m_hover, m_press, m_release;
    std::function<void()> m_cancel;
};
}

WindowPicker::WindowPicker(QObject *parent) : QObject(parent) {
    m_hoverTimer.setInterval(100);
    connect(&m_hoverTimer, &QTimer::timeout, this, [this] {
        refreshWindows();
        updateHover(QCursor::pos());
    });
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(60000);
    connect(&m_timeout, &QTimer::timeout, this, [this] { finish(); });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this] { finish(); });
    connect(qApp, &QGuiApplication::screenAdded, this, [this] { finish(); });
}

WindowPicker::~WindowPicker() { qDeleteAll(m_overlays); }
bool WindowPicker::isSupported() { return nativeWindowPickingSupported(); }

void WindowPicker::start() {
    QWidget *focusOverlay = nullptr;
    for (auto *screen : QGuiApplication::screens()) {
        auto *overlay = new PickerOverlay(screen,
            [this](const QPointF &point) { updateHover(point); },
            [this](const QPointF &point) { press(point); },
            [this](const QPointF &point) { release(point); }, [this] { finish(); });
        m_overlays.append(overlay);
        overlay->show();
        if (screen->geometry().contains(QCursor::pos())) focusOverlay = overlay;
        connect(screen, &QScreen::geometryChanged, this, [this] { finish(); });
    }
    if (!focusOverlay && !m_overlays.isEmpty()) focusOverlay = m_overlays.first();
    if (focusOverlay) {
        focusOverlay->raise();
        focusOverlay->activateWindow();
        focusOverlay->setFocus();
    }
    refreshWindows();
    updateHover(QCursor::pos());
    m_hoverTimer.start();
    m_timeout.start();
}

void WindowPicker::refreshWindows() {
    // A headless Qt platform has no desktop window server. Qt 6.8's CG backend
    // assumes CGWindowListCopyWindowInfo succeeds, so never query it there.
    if (!isSupported()) {
        m_windows.clear();
        m_captureWindows.clear();
        return;
    }
    m_windows = nativePickerWindows();
    m_captureWindows = QWindowCapture::capturableWindows();
}

void WindowPicker::updateHover(const QPointF &globalPosition) {
    if (m_finished) return;
    auto *screen = QGuiApplication::screenAt(globalPosition.toPoint());
    m_hover = screen ? pickerWindowAt(m_windows,
        pickerNativePoint(globalPosition, screen->geometry(), pickerNativeScale(screen))) : NativeWindowCandidate();
    m_hoverWindow = capturableWindowWithId(m_captureWindows, m_hover.id);
    const bool capturable = m_hoverWindow.isValid();
    for (auto *overlay : m_overlays)
        static_cast<PickerOverlay *>(overlay)->showCandidate(m_hover, capturable, globalPosition);
}

void WindowPicker::press(const QPointF &globalPosition) {
    if (m_finished) return;
    refreshWindows();
    updateHover(globalPosition);
    m_pressedId = m_hoverWindow.isValid() ? m_hover.id : 0;
}

void WindowPicker::release(const QPointF &globalPosition) {
    if (m_finished) return;
    const auto pressedId = std::exchange(m_pressedId, 0);
    refreshWindows();
    updateHover(globalPosition);
    // Never select a different window if the pointer moved, stacking changed, or
    // the pressed window disappeared. The highlighted ID and selected ID agree.
    if (pressedId && pressedId == m_hover.id && m_hoverWindow.isValid()) finish(m_hoverWindow);
}

void WindowPicker::finish(const QCapturableWindow &window) {
    if (m_finished) return;
    m_finished = true;
    m_hoverTimer.stop();
    m_timeout.stop();
    // Copy before emitting: callbacks can enumerate windows or process other events.
    const auto selected = window;
    emit finished(selected);
    for (auto *overlay : m_overlays) overlay->hide();
    deleteLater();
}
