#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEST_DIR="$(mktemp -d "${TMPDIR:-/tmp}/clipdock-sign-test.XXXXXX")"
trap 'rm -rf "$TEST_DIR"' EXIT
APP="$TEST_DIR/SigningTest.app"
mkdir -p "$APP/Contents/MacOS"
cat > "$TEST_DIR/main.cpp" <<'SOURCE'
int main() { return 0; }
SOURCE
cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleIdentifier</key><string>local.clipdock.signingtest</string>
<key>CFBundleExecutable</key><string>SigningTest</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleVersion</key><string>1</string>
</dict></plist>
PLIST
/usr/bin/clang++ "$TEST_DIR/main.cpp" -o "$APP/Contents/MacOS/SigningTest"
export CODE_SIGN_IDENTITY=-
bash "$ROOT/scripts/sign-macos-app.sh" "$APP"
/usr/bin/codesign --verify --deep --strict "$APP"
DETAILS="$(/usr/bin/codesign -dv --verbose=2 "$APP" 2>&1)"
grep -Fxq 'Identifier=local.clipdock.signingtest' <<< "$DETAILS"
BEFORE="$(shasum -a 256 "$APP/Contents/MacOS/SigningTest")"
OUTPUT="$(bash "$ROOT/scripts/sign-macos-app.sh" "$APP")"
[[ -z "$OUTPUT" ]]
[[ "$BEFORE" == "$(shasum -a 256 "$APP/Contents/MacOS/SigningTest")" ]]

# A valid signature with the wrong ID must be repaired, not accepted.
/usr/bin/codesign --force --sign - --identifier wrong.identifier "$APP"
bash "$ROOT/scripts/sign-macos-app.sh" "$APP"
DETAILS="$(/usr/bin/codesign -dv --verbose=2 "$APP" 2>&1)"
grep -Fxq 'Identifier=local.clipdock.signingtest' <<< "$DETAILS"

# Info.plist is sealed: changing it invalidates the old bundle signature.
/usr/libexec/PlistBuddy -c 'Set :CFBundleVersion 2' "$APP/Contents/Info.plist"
if /usr/bin/codesign --verify --deep --strict "$APP" 2>/dev/null; then
    echo 'FAIL: changed Info.plist unexpectedly accepted' >&2
    exit 1
fi
bash "$ROOT/scripts/sign-macos-app.sh" "$APP"
/usr/bin/codesign --verify --deep --strict "$APP"

# Finder metadata can be copied into deployed frameworks/resources (including
# cloud-synced checkouts) and makes an otherwise valid bundle unsignable.
mkdir -p "$APP/Contents/Resources/nested"
printf 'fixture\n' > "$APP/Contents/Resources/nested/data.txt"
/usr/bin/xattr -wx com.apple.FinderInfo 0000000000000000000000000000000000000000000000000000000000000000 "$APP/Contents/Resources/nested"
/usr/bin/xattr -w com.apple.ResourceFork test "$APP/Contents/Resources/nested/data.txt"
/usr/bin/xattr -w local.clipdock.preserve test "$APP/Contents/Resources/nested/data.txt"
bash "$ROOT/scripts/sign-macos-app.sh" "$APP"
/usr/bin/codesign --verify --deep --strict "$APP"
[[ "$(/usr/bin/xattr -p local.clipdock.preserve "$APP/Contents/Resources/nested/data.txt")" == test ]]
echo 'PASS: bundle signing, matching ID, unchanged signature, changed bundle and Finder metadata repair'
