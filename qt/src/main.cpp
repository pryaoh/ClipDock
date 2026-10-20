#include "MainWindow.h"
#include <QApplication>
#include <QByteArray>

int main(int argc, char *argv[]) {
    // QScreenCapture and QWindowCapture require the FFmpeg multimedia backend.
    qputenv("QT_MEDIA_BACKEND", QByteArrayLiteral("ffmpeg"));
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ClipDock"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setOrganizationName(QStringLiteral("ClipDock"));
    app.setQuitOnLastWindowClosed(true);
    MainWindow window;
    window.show();
    return app.exec();
}
