#include "RecordingBar.h"
#include "UiTheme.h"
#include <QCloseEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QWindow>

RecordingBar::RecordingBar()
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
    setObjectName(QStringLiteral("recordingBar"));
    setWindowTitle(QStringLiteral("ClipDock 녹화 컨트롤바"));
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_MacAlwaysShowToolWindow);
    setCursor(Qt::SizeAllCursor);
    setStyleSheet(QStringLiteral(
        "QWidget#recordingBar { background: #182335; border: 1px solid #59677a; }"
        "QLabel { color: #eef2f8; }"
        "QPushButton { background: #304059; color: white; border: 1px solid #65748a;"
        " border-radius: 4px; padding: 4px 6px; }"
        "QPushButton:hover, QPushButton:focus { background: #495e7c; }"
        "QPushButton:pressed { background: #233147; }"));
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(8, 5, 8, 5);
    row->setSpacing(6);
    m_status = new QLabel;
    m_status->setAccessibleDescription(QStringLiteral("드래그하여 이동"));
    m_status->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_time = new QLabel(QStringLiteral("00:00:00"));
    m_time->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_time->setAccessibleName(QStringLiteral("녹화 경과 시간"));
    m_time->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_microphone = new QLabel;
    row->addWidget(m_status);
    row->addWidget(m_time);
    row->addWidget(m_microphone);
    auto addButton = [&](const QString &label, UiTheme::Icon icon) {
        auto *button = new QPushButton(UiTheme::icon(icon, Qt::white), label);
        button->setAccessibleName(label);
        button->setCursor(Qt::PointingHandCursor);
        row->addWidget(button);
        return button;
    };
    m_pause = addButton(QStringLiteral("일시정지"), UiTheme::Icon::Pause);
    connect(m_pause, &QPushButton::clicked, this, &RecordingBar::pauseRequested);
    connect(addButton(QStringLiteral("중지 및 저장"), UiTheme::Icon::Stop),
            &QPushButton::clicked, this, &RecordingBar::stopRequested);
    connect(addButton(QStringLiteral("메인 창"), UiTheme::Icon::Window),
            &QPushButton::clicked, this, &RecordingBar::restoreRequested);
    auto *hideButton = new QPushButton(QStringLiteral("숨기기"));
    hideButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(hideButton);
    connect(hideButton, &QPushButton::clicked, this, &QWidget::close);
}

void RecordingBar::closeEvent(QCloseEvent *event) {
    emit dismissed();
    event->accept();
}

void RecordingBar::setRecordingState(bool paused, bool microphone) {
    m_status->setText(paused ? QStringLiteral("Ⅱ 일시정지") : QStringLiteral("● 녹화 중"));
    m_status->setStyleSheet(paused ? QStringLiteral("color: #ffd280") : QStringLiteral("color: #ffada8"));
    m_microphone->setText(microphone ? QStringLiteral("마이크 켬") : QStringLiteral("마이크 끔"));
    const QString action = paused ? QStringLiteral("재개") : QStringLiteral("일시정지");
    m_pause->setText(action);
    m_pause->setAccessibleName(action);
    m_pause->setIcon(UiTheme::icon(paused ? UiTheme::Icon::Play : UiTheme::Icon::Pause, Qt::white));
    setFixedSize(sizeHint());
}

void RecordingBar::setDuration(const QString &duration) {
    m_time->setText(duration);
}

bool RecordingBar::place(QScreen *preferredScreen, const QRect &captureToAvoid) {
    m_captureToAvoid = captureToAvoid;
    auto screens = QGuiApplication::screens();
    if (m_position) {
        if (auto *previous = QGuiApplication::screenAt(*m_position)) preferredScreen = previous;
    }
    if (screens.removeOne(preferredScreen)) screens.prepend(preferredScreen);
    for (auto *display : screens) {
        const QRect area = display->availableGeometry();
        const int right = area.right() - width() + 1;
        const int bottom = area.bottom() - height() + 1;
        if (right < area.left() || bottom < area.top()) continue;
        const QList<QPoint> candidates = {
            m_position.value_or(QPoint(right - 12, area.top() + 12)),
            QPoint(right, area.top()), area.topLeft(), QPoint(right, bottom), QPoint(area.left(), bottom)
        };
        for (auto point : candidates) {
            point.setX(qBound(area.left(), point.x(), right));
            point.setY(qBound(area.top(), point.y(), bottom));
            if (QRect(point, size()).intersects(captureToAvoid)) continue;
            move(point);
            m_position = point;
            return true;
        }
    }
    return false;
}

void RecordingBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragOffset = event->globalPosition().toPoint() - pos();
        if (QGuiApplication::platformName().startsWith(QStringLiteral("wayland")))
            windowHandle()->startSystemMove();
    }
}

void RecordingBar::mouseMoveEvent(QMouseEvent *event) {
    if (!(event->buttons() & Qt::LeftButton)) return;
    const auto cursor = event->globalPosition().toPoint();
    auto *display = QGuiApplication::screenAt(cursor);
    if (!display) return;
    const QRect area = display->availableGeometry();
    if (area.width() < width() || area.height() < height()) return;
    const QPoint desired = cursor - m_dragOffset;
    const QPoint next(qBound(area.left(), desired.x(), area.right() - width() + 1),
                      qBound(area.top(), desired.y(), area.bottom() - height() + 1));
    if (QRect(next, size()).intersects(m_captureToAvoid)) return;
    move(next);
    m_position = next;
}
