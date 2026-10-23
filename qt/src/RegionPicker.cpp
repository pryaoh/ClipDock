#include "RegionPicker.h"
#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>
#include <algorithm>

RegionPicker::RegionPicker(QScreen *screen)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_screen(screen), m_hint(new QLabel(this)) {
    setObjectName(QStringLiteral("regionPicker"));
    setWindowTitle(QStringLiteral("녹화 영역 지정"));
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    if (screen) {
        setGeometry(screen->geometry());
        connect(screen, &QScreen::geometryChanged, this, [this] { finish(); });
        connect(screen, &QObject::destroyed, this, [this] { finish(); });
    }
    connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *removed) {
        if (!m_screen || removed == m_screen) finish();
    });
    m_hint->setText(QStringLiteral("녹화할 영역을 드래그하세요 · 최소 16 × 16\nEsc / 오른쪽 클릭: 취소"));
    m_hint->setAlignment(Qt::AlignCenter);
    m_hint->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hint->setStyleSheet(QStringLiteral("background: #17212e; color: white; border: 2px solid #58a6ff;"
                                        "border-radius: 8px; padding: 10px; font-size: 16px;"));
    m_hint->setFixedSize(qMin(650, qMax(1, width() - 32)), 80);
    m_hint->move((width() - m_hint->width()) / 2, 24);
}

void RegionPicker::start() {
    if (!m_screen) { finish(); return; }
    show();
    raise();
    activateWindow();
    setFocus();
    QTimer::singleShot(60000, this, [this] { finish(); });
}

void RegionPicker::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 1)); // Keep the whole overlay hit-testable.
    QPainterPath shade;
    shade.setFillRule(Qt::OddEvenFill);
    shade.addRect(rect());
    if (!m_region.isEmpty()) shade.addRect(m_region);
    painter.fillPath(shade, QColor(0, 0, 0, 95));
    if (!m_region.isEmpty()) {
        painter.setPen(QPen(QColor(45, 150, 255), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(m_region).adjusted(1, 1, -1, -1));
    }
}

void RegionPicker::updateSelection(const QPointF &point) {
    const QPointF clipped(std::clamp(point.x(), 0.0, double(width())),
                          std::clamp(point.y(), 0.0, double(height())));
    m_region = QRectF(m_anchor, clipped).normalized().toAlignedRect().intersected(rect());
    m_hint->setText(QStringLiteral("%1 × %2 · 놓으면 선택 · 최소 16 × 16\nEsc / 오른쪽 클릭: 취소")
        .arg(m_region.width()).arg(m_region.height()));
    m_hint->move((width() - m_hint->width()) / 2, point.y() < 130 ? qMax(0, height() - 104) : 24);
    update();
}

void RegionPicker::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) { finish(); return; }
    if (event->button() != Qt::LeftButton) return;
    m_anchor = event->position();
    m_dragging = true;
    updateSelection(m_anchor);
}
void RegionPicker::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) updateSelection(event->position());
}
void RegionPicker::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !m_dragging) return;
    m_dragging = false;
    updateSelection(event->position());
    if (m_region.width() >= 16 && m_region.height() >= 16) finish(m_region);
    else m_hint->setText(QStringLiteral("영역이 너무 작습니다. 16 × 16 이상으로 다시 드래그하세요.\nEsc / 오른쪽 클릭: 취소"));
}
void RegionPicker::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) finish();
    else QWidget::keyPressEvent(event);
}
void RegionPicker::closeEvent(QCloseEvent *event) { event->accept(); finish(); }
void RegionPicker::finish(const QRect &region) {
    if (m_finished) return;
    m_finished = true;
    const auto selected = region;
    hide(); // Never include the selection overlay in a captured frame.
    emit finished(selected);
    deleteLater();
}

RegionSelection::RegionSelection(QObject *parent) : QObject(parent) {
    connect(qApp, &QGuiApplication::screenAdded, this, [this] { finish(); });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this] { finish(); });
}

RegionSelection::~RegionSelection() { qDeleteAll(m_overlays); }

void RegionSelection::start(QScreen *preferredScreen) {
    if (m_finished || !m_overlays.isEmpty()) return;
    RegionPicker *focusOverlay = nullptr;
    for (auto *screen : QGuiApplication::screens()) {
        auto *overlay = new RegionPicker(screen);
        m_overlays.append(overlay);
        const QPointer<QScreen> selectedScreen = screen;
        connect(overlay, &RegionPicker::finished, this, [this, selectedScreen](const QRect &region) {
            if (selectedScreen && !region.isEmpty()) finish(selectedScreen, region);
            else finish();
        });
        overlay->start();
        if (screen == preferredScreen) focusOverlay = overlay;
    }
    if (m_overlays.isEmpty()) { finish(); return; }
    if (!focusOverlay) focusOverlay = m_overlays.front();
    focusOverlay->raise();
    focusOverlay->activateWindow();
    focusOverlay->setFocus();
}

void RegionSelection::finish(QScreen *screen, const QRect &region) {
    if (m_finished) return;
    m_finished = true;
    const auto selected = region;
    // Other monitors must also be clear before the UI can start recording.
    for (auto *overlay : m_overlays) overlay->hide();
    emit finished(screen, selected);
    deleteLater();
}
