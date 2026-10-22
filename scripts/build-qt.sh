#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/scripts/qt-sdk.sh"
DEFAULT_BUILD="$ROOT/build/qt"
if [[ "$(uname -s)" == Linux ]]; then DEFAULT_BUILD="$ROOT/build/qt-linux"; fi
BUILD="${BUILD_DIR:-$DEFAULT_BUILD}"
CONFIG="${CONFIGURATION:-Release}"
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake 3.24 이상이 필요합니다. macOS: brew install cmake ninja / Ubuntu 24.04: sudo apt install cmake ninja-build build-essential" >&2
    exit 1
fi
ARGS=(-S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE="$CONFIG" -DBUILD_TESTING=ON)
QT_PREFIX="$(qt_sdk_select "$ROOT")"
if [[ -n "$QT_PREFIX" ]]; then
    if [[ ! -f "$QT_PREFIX/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
        echo "Qt SDK 경로가 올바르지 않습니다: $QT_PREFIX" >&2
        exit 1
    fi
    # Qt package cache entries are absolute paths; do not mix Homebrew and official kits.
    PREVIOUS_QT="$(cat "$BUILD/.screenencoder-qt-root" 2>/dev/null || true)"
    if [[ "$PREVIOUS_QT" != "$QT_PREFIX" ]]; then ARGS+=(--fresh); fi
    ARGS+=("-DCMAKE_PREFIX_PATH=$QT_PREFIX" "-DQt6_DIR=$QT_PREFIX/lib/cmake/Qt6")
fi
cmake "${ARGS[@]}" "$@"
if [[ -n "$QT_PREFIX" ]]; then printf '%s\n' "$QT_PREFIX" > "$BUILD/.screenencoder-qt-root"; fi
cmake --build "$BUILD" --config "$CONFIG" --parallel
if [[ "$(uname -s)" == Darwin ]]; then
    # Also repair older already-built bundles when no relink was necessary.
    bash "$ROOT/scripts/sign-macos-app.sh" "$BUILD/ClipDock.app"
fi
echo "빌드 완료: $BUILD"
