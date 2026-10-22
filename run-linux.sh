#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
if [[ "$(uname -s)" != Linux ]]; then
    echo "Linux에서 실행하세요." >&2
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1 || ! command -v c++ >/dev/null 2>&1; then
    echo "CMake 3.24+와 C++20 컴파일러가 필요합니다. Ubuntu 24.04: sudo apt install build-essential cmake ninja-build" >&2
    exit 1
fi
source "$ROOT/scripts/qt-sdk.sh"
SELECTED_QT="$(qt_sdk_select "$ROOT")"
if ! qt_sdk_has_ffmpeg "$SELECTED_QT"; then
    if [[ -n "${QT_ROOT:-}" ]]; then
        echo "QT_ROOT의 Qt에는 화면 캡처에 필요한 FFmpeg 플러그인이 없습니다: $QT_ROOT" >&2
        exit 1
    fi
    bash "$ROOT/scripts/setup-linux-qt.sh"
    SELECTED_QT="$(qt_sdk_select "$ROOT")"
fi
export QT_ROOT="$SELECTED_QT"
bash "$ROOT/scripts/build-qt.sh"
exec "${BUILD_DIR:-$ROOT/build/qt-linux}/ClipDock" "$@"
