#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "macOS에서 실행하세요." >&2
    exit 1
fi
CONFIG="${CONFIGURATION:-release}"
if [[ "$CONFIG" != release && "$CONFIG" != debug ]]; then
    echo "CONFIGURATION은 release 또는 debug여야 합니다." >&2
    exit 1
fi
export CLANG_MODULE_CACHE_PATH="$ROOT/macOS/.build/ModuleCache"
export SWIFTPM_MODULECACHE_OVERRIDE="$CLANG_MODULE_CACHE_PATH"
mkdir -p "$CLANG_MODULE_CACHE_PATH"
SWIFT_OPTIONS=(--package-path "$ROOT/macOS" --disable-sandbox --cache-path "$ROOT/macOS/.build/cache" --config-path "$ROOT/macOS/.build/config" --security-path "$ROOT/macOS/.build/security" --manifest-cache local)
swift build "${SWIFT_OPTIONS[@]}" --product ScreenEncoder -c "$CONFIG" "$@"
BIN="$(swift build "${SWIFT_OPTIONS[@]}" -c "$CONFIG" --show-bin-path "$@")"
APP="$ROOT/dist/ScreenEncoder.app"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN/ScreenEncoder" "$APP/Contents/MacOS/ScreenEncoder"
cp "$ROOT/macOS/Info.plist" "$APP/Contents/Info.plist"
plutil -lint "$APP/Contents/Info.plist"
codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$APP"
codesign --verify --strict "$APP"
echo "빌드 완료: $APP"
