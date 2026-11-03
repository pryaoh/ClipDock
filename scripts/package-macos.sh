#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "macOS에서 실행하세요." >&2
    exit 1
fi
"$ROOT/scripts/build-qt.sh" "$@"
PACKAGE="${PACKAGE_DIR:-$ROOT/dist/qt}"
mkdir -p "$PACKAGE"
PACKAGE="$(cd "$PACKAGE" && pwd -P)"
# File Provider can recreate FinderInfo on framework directories immediately
# after xattr removes it. Sign outside cloud storage and deliver a sealed ZIP.
case "$PACKAGE" in
    "$HOME/Library/CloudStorage/"*)
        PACKAGE="$(mktemp -d "${TMPDIR:-/tmp}/clipdock-package.XXXXXX")"
        echo "클라우드 동기화 메타데이터를 피하기 위해 로컬 폴더에서 패키징합니다: $PACKAGE"
        ;;
esac
cmake --install "${BUILD_DIR:-$ROOT/build/qt}" --config "${CONFIGURATION:-Release}" --prefix "$PACKAGE"
APP="$PACKAGE/ClipDock.app"
bash "$ROOT/scripts/clean-macos-signing-metadata.sh" "$APP"
# Sign embedded frameworks/plugins after Qt's deployment step, then sign the app.
while IFS= read -r -d '' binary; do
    codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$binary"
done < <(find "$APP/Contents" -type f \( -name '*.dylib' -o -perm -111 \) -print0)
while IFS= read -r -d '' framework; do
    codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$framework"
done < <(find "$APP/Contents" -type d -name '*.framework' -print0)
codesign --force --sign "${CODE_SIGN_IDENTITY:--}" "$APP"
codesign --verify --deep --strict "$APP"
mkdir -p "$ROOT/dist"
ditto -c -k --sequesterRsrc --keepParent "$APP" "$ROOT/dist/ClipDock-macOS.zip"
echo "실행 파일: $APP"
echo "배포 압축 파일: $ROOT/dist/ClipDock-macOS.zip"
