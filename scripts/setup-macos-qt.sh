#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/scripts/qt-sdk.sh"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "macOS에서 실행하세요." >&2
    exit 1
fi
SDK="$ROOT/.tools/qt/$CLIPDOCK_QT_VERSION/macos"
if qt_sdk_has_ffmpeg "$SDK"; then
    echo "화면 캡처용 Qt 준비 완료: $SDK"
    exit 0
fi
echo "FFmpeg 백엔드가 포함된 Qt $CLIPDOCK_QT_VERSION SDK를 프로젝트의 .tools/qt에 설치합니다."
echo "최초 실행에는 인터넷 연결이 필요합니다. 기존 Homebrew Qt와 Swift 보관본은 변경하지 않습니다."
INSTALLER="$ROOT/.tools/qt-installer"
if [[ ! -x "$INSTALLER/bin/python" ]]; then
    python3 -m venv "$INSTALLER"
fi
if ! "$INSTALLER/bin/python" -c 'import aqt' >/dev/null 2>&1; then
    if ! "$INSTALLER/bin/python" -m pip --disable-pip-version-check install --no-cache-dir 'aqtinstall>=3.3,<4'; then
        echo "Qt 설치 도구 다운로드 실패: 인터넷/DNS 연결을 확인한 뒤 ./run-macos.command를 다시 실행하세요." >&2
        exit 1
    fi
fi
if ! "$INSTALLER/bin/python" -m aqt install-qt mac desktop "$CLIPDOCK_QT_VERSION" clang_64 \
    --outputdir "$ROOT/.tools/qt" --modules qtmultimedia qtshadertools; then
    echo "Qt SDK 다운로드 실패: 인터넷 연결을 확인하고 다시 실행하세요." >&2
    exit 1
fi
if ! qt_sdk_has_ffmpeg "$SDK"; then
    echo "Qt 설치에 FFmpeg 미디어 플러그인이 없습니다: $SDK" >&2
    exit 1
fi
echo "화면 캡처용 Qt 준비 완료: $SDK"
