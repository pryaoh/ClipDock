#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "macOS에서 실행하세요." >&2
    exit 1
fi
"$ROOT/scripts/build-qt.sh" "$@"
cmake --install "$ROOT/build/qt" --config "${CONFIGURATION:-Release}" --prefix "$ROOT/dist/qt"
APP="$ROOT/dist/qt/ClipDock.app"
# Sign embedded frameworks/plugins after Qt's deployment step, then sign the app.
while IFS= read -r -d '' binary; do
    codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$binary"
done < <(find "$APP/Contents" -type f \( -name '*.dylib' -o -perm -111 \) -print0)
while IFS= read -r -d '' framework; do
    codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$framework"
done < <(find "$APP/Contents" -type d -name '*.framework' -print0)
codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$APP"
codesign --verify --deep --strict "$APP"
echo "실행 파일: $APP"
