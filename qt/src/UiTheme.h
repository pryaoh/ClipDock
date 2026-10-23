#pragma once

#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QWidget>

namespace UiTheme {

enum class Icon { Screen, Record, Play, Pause, Stop, Folder, Refresh, Region, Window };

// Render small vector icons at several sizes so they remain crisp on HiDPI screens.
inline QIcon icon(Icon kind, const QColor &color = QColor(QStringLiteral("#52627a"))) {
    QIcon result;
    for (auto mode : {QIcon::Normal, QIcon::Disabled}) {
        const QColor ink = mode == QIcon::Disabled ? QColor(QStringLiteral("#aeb8c6")) : color;
        for (int size : {16, 20, 24, 32, 48, 64, 96, 128}) {
            QPixmap pixmap(size, size);
            pixmap.fill(Qt::transparent);
            QPainter p(&pixmap);
            p.setRenderHint(QPainter::Antialiasing);
            p.scale(size / 24.0, size / 24.0);
            p.setPen(QPen(ink, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            switch (kind) {
            case Icon::Screen:
                p.drawRoundedRect(QRectF(3, 4, 18, 13), 2, 2);
                p.drawLine(QPointF(12, 17), QPointF(12, 21));
                p.drawLine(QPointF(8, 21), QPointF(16, 21));
                break;
            case Icon::Record:
                p.setPen(Qt::NoPen); p.setBrush(ink);
                p.drawEllipse(QRectF(5, 5, 14, 14));
                break;
            case Icon::Play: {
                QPainterPath path;
                path.moveTo(8, 5); path.lineTo(19, 12); path.lineTo(8, 19); path.closeSubpath();
                p.setBrush(ink); p.drawPath(path);
                break;
            }
            case Icon::Pause:
                p.setPen(Qt::NoPen); p.setBrush(ink);
                p.drawRoundedRect(QRectF(6, 5, 4, 14), 1, 1);
                p.drawRoundedRect(QRectF(14, 5, 4, 14), 1, 1);
                break;
            case Icon::Stop:
                p.setPen(Qt::NoPen); p.setBrush(ink);
                p.drawRoundedRect(QRectF(5, 5, 14, 14), 2, 2);
                break;
            case Icon::Folder: {
                QPainterPath path;
                path.moveTo(3, 8); path.lineTo(3, 6); path.lineTo(9, 6); path.lineTo(11, 8);
                path.lineTo(21, 8); path.lineTo(21, 19); path.lineTo(3, 19); path.closeSubpath();
                p.drawPath(path);
                break;
            }
            case Icon::Refresh:
                p.drawArc(QRectF(5, 5, 14, 14), 45 * 16, 290 * 16);
                p.drawLine(QPointF(18, 3), QPointF(18, 8));
                p.drawLine(QPointF(13, 8), QPointF(18, 8));
                break;
            case Icon::Region:
                for (int angle : {0, 90, 180, 270}) {
                    p.save(); p.translate(12, 12); p.rotate(angle);
                    p.drawLine(QPointF(-8, -3), QPointF(-8, -8));
                    p.drawLine(QPointF(-8, -8), QPointF(-3, -8)); p.restore();
                }
                break;
            case Icon::Window:
                p.drawRoundedRect(QRectF(3, 4, 18, 16), 2, 2);
                p.drawLine(QPointF(3, 9), QPointF(21, 9));
                p.drawPoint(QPointF(6, 6.5));
                break;
            }
            p.end();
            result.addPixmap(pixmap, mode);
        }
    }
    return result;
}

inline void applyLightTheme(QWidget *window) {
    // Explicit light colors also keep popups legible when the OS uses dark mode.
    auto palette = window->palette();
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#f4f6fa")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#24324a")));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f4f6fa")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#24324a")));
    palette.setColor(QPalette::Button, Qt::white);
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#24324a")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#2563eb")));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#24324a")));
    for (auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText})
        palette.setColor(QPalette::Disabled, role, QColor(QStringLiteral("#9aa5b5")));
    window->setPalette(palette);
    window->setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#appSurface { background: #f4f6fa; }
        QLabel { color: #24324a; font-size: 13px; background: transparent; }
        QLabel#appTitle { font-size: 26px; font-weight: 700; }
        QLabel#subtitle, QLabel#fieldLabel { color: #68778d; font-size: 12px; }
        QLabel#brandIcon { background: #2563eb; border-radius: 12px; }
        QLabel#cardTitle { font-size: 15px; font-weight: 600; }
        QLabel#hint, QLabel#regionHint { color: #748198; font-size: 11px; }
        QLabel#previewCaption { color: #748198; font-size: 12px; }
        QLabel#emptyTitle { font-size: 19px; font-weight: 600; }
        QLabel#emptyHint { color: #748198; font-size: 13px; }
        QLabel#emptyIcon { background: #eaf0ff; border-radius: 20px; }
        QLabel#timer { font-size: 28px; font-weight: 600; color: #26344d; }
        QLabel#statusText { color: #68778d; font-size: 12px; }
        QLabel#stateBadge { background: #edf2f8; color: #52627a; border-radius: 13px;
                           padding: 6px 12px; font-size: 12px; font-weight: 600; }
        QLabel#stateBadge[tone="active"] { background: #eaf0ff; color: #2563eb; }
        QLabel#stateBadge[tone="recording"] { background: #fff0f1; color: #d43b50; }
        QLabel#stateBadge[tone="paused"] { background: #fff5df; color: #9d680e; }
        QFrame#previewCard, QFrame#settingsCard, QFrame#controlBar {
            background: white; border: 1px solid #e1e7f0; border-radius: 14px;
        }
        QFrame#separator { background: #e9edf4; border: none; max-height: 1px; }
        QWidget#emptyPreview { background: #f8faff; border: 1px dashed #dce4f0; border-radius: 10px; }
        QStackedWidget#previewStack { border: none; background: transparent; }
        QVideoWidget { background: #131a25; }
        QComboBox, QSpinBox {
            background: #f9fafc; color: #24324a; border: 1px solid #dfe5ee; border-radius: 7px;
            min-height: 18px; padding: 7px 10px; font-size: 12px;
            selection-background-color: #2563eb; selection-color: white;
        }
        QComboBox { padding-right: 28px; }
        QComboBox:hover, QSpinBox:hover { border-color: #b5c4db; }
        QComboBox:focus, QSpinBox:focus { border-color: #2563eb; background: white; }
        QComboBox:disabled, QSpinBox:disabled { color: #9aa5b5; background: #f3f5f8; }
        QComboBox::drop-down { border: none; width: 26px; }
        QComboBox::down-arrow, QSpinBox::down-arrow {
            image: url(:/clipdock/chevron-down.xpm); width: 12px; height: 8px;
        }
        QSpinBox { padding-right: 24px; }
        QSpinBox::up-button, QSpinBox::down-button {
            subcontrol-origin: border; width: 20px; border: none; background: transparent;
        }
        QSpinBox::up-button { subcontrol-position: top right; }
        QSpinBox::down-button { subcontrol-position: bottom right; }
        QSpinBox::up-arrow {
            image: url(:/clipdock/chevron-up.xpm); width: 12px; height: 8px;
        }
        QComboBox QAbstractItemView {
            background: white; color: #24324a; border: 1px solid #dfe5ee;
            selection-background-color: #eaf0ff; selection-color: #1d4ed8; padding: 4px;
        }
        QPushButton {
            color: #52627a; background: white; border: 1px solid #dfe5ee; border-radius: 8px;
            min-height: 18px; padding: 8px 12px; font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #f5f8ff; border-color: #b7c9eb; color: #2563eb; }
        QPushButton:pressed { background: #eaf0ff; }
        QPushButton:focus { border-color: #2563eb; }
        QPushButton#previewButton { color: #2563eb; background: #edf3ff; border-color: #dbe7ff; }
        QPushButton#previewButton:hover { background: #e0ebff; border-color: #a9c4ff; }
        QPushButton#recordButton { background: #e24c61; color: white; border-color: #e24c61; padding: 10px 18px; }
        QPushButton#recordButton:hover { background: #cf3c51; border-color: #cf3c51; }
        QPushButton#recordButton:pressed { background: #b92f43; }
        QPushButton#recordButton:focus { border: 2px solid #8f2033; padding: 9px 17px; }
        QPushButton:disabled, QPushButton#previewButton:disabled, QPushButton#recordButton:disabled {
            color: #a4afbd; background: #f3f5f8; border-color: #e7ebf1;
        }
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical { background: transparent; width: 6px; margin: 2px 0; }
        QScrollBar::handle:vertical { background: #d8dfeb; border-radius: 3px; min-height: 32px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        QMenuBar { background: #f4f6fa; color: #52627a; font-size: 12px; }
        QMenuBar::item:selected { background: #e8eef8; border-radius: 4px; }
        QMenu { background: white; color: #24324a; border: 1px solid #dfe5ee; padding: 5px; }
        QMenu::item { padding: 6px 24px; }
        QMenu::item:selected { background: #edf3ff; color: #2563eb; }
        QToolTip { background: white; color: #24324a; border: 1px solid #dfe5ee; padding: 5px; }
    )"));
}

} // namespace UiTheme
