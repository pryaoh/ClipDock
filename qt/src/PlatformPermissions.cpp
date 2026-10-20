#include "PlatformPermissions.h"
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QUrl>
#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#endif

bool requestScreenPermission(QString &message) {
#ifdef Q_OS_MACOS
    if (!CGPreflightScreenCaptureAccess() && !CGRequestScreenCaptureAccess()) {
        message = QStringLiteral("시스템 설정 → 개인정보 보호 및 보안 → 화면 및 시스템 오디오 기록에서 "
                                 "ClipDock를 허용한 뒤 앱을 완전히 종료하고 다시 실행하세요.\n\n"
                                 "이미 켜져 있는데도 이 메시지가 표시되면, 이전 빌드나 다른 경로의 앱에 "
                                 "권한이 연결되어 있을 수 있습니다. 현재 앱을 다시 등록하세요.\n\n")
            + screenPermissionDiagnostics();
        return false;
    }
#else
    Q_UNUSED(message);
#endif
    return true;
}

QString screenPermissionDiagnostics() {
#ifdef Q_OS_MACOS
    const auto bundle = CFBundleGetMainBundle();
    const auto identifier = bundle ? CFBundleGetIdentifier(bundle) : nullptr;
    const auto bundleId = identifier ? QString::fromCFString(identifier) : QStringLiteral("확인 불가");
    const auto appPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../.."));
    const auto state = CGPreflightScreenCaptureAccess()
        ? QStringLiteral("허용됨")
        : QStringLiteral("현재 프로세스에 허용되지 않음 (권한 변경 후 재실행 필요 가능)");
    return QStringLiteral("화면 기록 권한: %1\n앱 ID: %2\n실행 중인 앱: %3\n\n"
                          "현재 버전은 시스템 소리 직접 녹음을 지원하지 않습니다. "
                          "선택한 마이크/루프백 입력 장치의 소리만 녹음합니다.")
        .arg(state, bundleId, appPath);
#else
    return QStringLiteral("이 운영체제의 화면 공유 선택 창과 입력 장치 설정을 확인하세요.");
#endif
}

void openScreenPermissionSettings() {
#ifdef Q_OS_MACOS
    QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture")));
#elif defined(Q_OS_WIN)
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:privacy-microphone")));
#endif
}
