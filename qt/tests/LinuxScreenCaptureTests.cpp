#include <QApplication>
#include <QImage>
#include <QMediaCaptureSession>
#include <QScreenCapture>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>
#include <QWidget>
#include <iostream>

// Run on an isolated Xvfb display, never on the user's desktop. The regular
// tests cover encoding; this checks the real Linux/X11 screen source too.
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QWidget window;
    window.resize(320, 240);
    window.setStyleSheet(QStringLiteral("background: #ce3434"));
    window.show();
    QScreenCapture capture;
    QMediaCaptureSession session;
    QVideoSink sink;
    session.setScreenCapture(&capture);
    session.setVideoSink(&sink);
    capture.setScreen(app.primaryScreen());
    int frames = 0;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (!frame.isValid() || frame.toImage().isNull()) return;
        if (++frames == 3) {
            std::cout << "PASS: Linux X11 captured 3 readable frames, "
                      << frame.width() << 'x' << frame.height() << '\n';
            capture.stop();
            app.exit(0);
        }
    });
    QObject::connect(&capture, &QScreenCapture::errorOccurred, &app, [&](auto, const QString &error) {
        std::cerr << "FAIL: " << error.toStdString() << '\n';
        app.exit(1);
    });
    QTimer::singleShot(10000, &app, [&] {
        std::cerr << "FAIL: no frames from the X11 capture source\n";
        app.exit(1);
    });
    QTimer::singleShot(0, &capture, &QScreenCapture::start);
    return app.exec();
}
