#!/usr/bin/env bash
set -euo pipefail

APP="${1:?Usage: sign-macos-app.sh /path/to/ClipDock.app}"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "macOS에서 실행하세요." >&2
    exit 1
fi
PLIST="$APP/Contents/Info.plist"
BUNDLE_ID="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$PLIST")"
if [[ -z "$BUNDLE_ID" ]]; then
    echo "앱의 CFBundleIdentifier가 없습니다: $APP" >&2
    exit 1
fi

# Linker-generated ad-hoc signatures seal only the Mach-O executable, not the
# app bundle or its Info.plist. TCC needs the actual app identity. Keep a valid
# existing bundle signature on unchanged builds instead of signing every run.
SIGNING_IDENTITY="${CODE_SIGN_IDENTITY:--}"
if [[ "$SIGNING_IDENTITY" == - ]] &&
   /usr/bin/codesign --verify --deep --strict "$APP" 2>/dev/null; then
    SIGNATURE_DETAILS="$(/usr/bin/codesign -dv --verbose=2 "$APP" 2>&1)"
    if /usr/bin/grep -Fxq "Identifier=$BUNDLE_ID" <<< "$SIGNATURE_DETAILS"; then
        exit 0
    fi
fi

# Do not weaken designated requirements to an identifier-only check. A local
# ad-hoc build may need consent again after its executable changes; developers
# can supply an Apple Development / Developer ID identity to keep that stable.
/usr/bin/codesign --force --sign "$SIGNING_IDENTITY" --identifier "$BUNDLE_ID" "$APP"
/usr/bin/codesign --verify --deep --strict "$APP"
echo "macOS 앱 서명 확인 완료: $APP ($BUNDLE_ID)"
