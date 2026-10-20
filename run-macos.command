#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
source "$ROOT/scripts/qt-sdk.sh"
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake가 필요합니다. brew install cmake ninja 실행 후 다시 시도하세요." >&2
    exit 1
fi
SELECTED_QT="$(qt_sdk_select "$ROOT")"
if ! qt_sdk_has_ffmpeg "$SELECTED_QT"; then
    if [[ -n "${QT_ROOT:-}" ]]; then
        echo "QT_ROOT의 Qt에는 화면 캡처에 필요한 FFmpeg 플러그인이 없습니다: $QT_ROOT" >&2
        echo "QT_ROOT를 해제하면 프로젝트 전용 Qt를 자동 설치합니다: unset QT_ROOT" >&2
        exit 1
    fi
    "$ROOT/scripts/setup-macos-qt.sh"
    SELECTED_QT="$(qt_sdk_select "$ROOT")"
fi
export QT_ROOT="$SELECTED_QT"
"$ROOT/scripts/build-qt.sh"
open "$ROOT/build/qt/ClipDock.app"
