#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/scripts/qt-sdk.sh"
if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
    echo "자동 설치는 Linux x86_64용입니다. 다른 아키텍처에서는 호환 Qt 6.8+ kit를 QT_ROOT로 지정하세요." >&2
    exit 1
fi
SDK="$ROOT/.tools/qt/$CLIPDOCK_QT_VERSION/gcc_64"
if qt_sdk_has_ffmpeg "$SDK"; then
    echo "화면 캡처용 Qt 준비 완료: $SDK"
    exit 0
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "Python 3와 venv가 필요합니다. Ubuntu: sudo apt install python3-venv" >&2
    exit 1
fi
INSTALLER="$ROOT/.tools/qt-installer-linux"
if [[ ! -x "$INSTALLER/bin/python" ]]; then
    python3 -m venv "$INSTALLER"
fi
if ! "$INSTALLER/bin/python" -c 'import aqt' >/dev/null 2>&1; then
    "$INSTALLER/bin/python" -m pip --disable-pip-version-check install 'aqtinstall>=3.3,<4'
fi
echo "공식 Qt $CLIPDOCK_QT_VERSION Linux SDK를 프로젝트의 .tools/qt에 설치합니다."
# qtwayland is included in the default desktop archives, not an add-on module.
"$INSTALLER/bin/python" -m aqt install-qt linux desktop "$CLIPDOCK_QT_VERSION" linux_gcc_64 \
    --outputdir "$ROOT/.tools/qt" --modules qtmultimedia qtshadertools
if ! qt_sdk_has_ffmpeg "$SDK"; then
    echo "Qt 설치에 FFmpeg 미디어 플러그인이 없습니다: $SDK" >&2
    exit 1
fi
echo "화면 캡처용 Qt 준비 완료: $SDK"
