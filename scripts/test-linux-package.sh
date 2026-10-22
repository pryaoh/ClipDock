#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FIXTURE="$(mktemp -d "${TMPDIR:-/tmp}/clipdock-package.XXXXXX")"
trap 'rm -rf "$FIXTURE"' EXIT
cp -a "$ROOT/dist/linux/." "$FIXTURE/"
# Prove the relocated package does not depend on the developer's Qt environment.
unset LD_LIBRARY_PATH QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH QML2_IMPORT_PATH QML_IMPORT_PATH
SDK="${QT_ROOT:-${QT_ROOT_DIR:-$ROOT/.tools/qt/6.8.3/gcc_64}}"
unset QT_ROOT QT_ROOT_DIR QTDIR
shopt -s nullglob
XCB=("$FIXTURE"/plugins/platforms/libqxcb.so)
WAYLAND=("$FIXTURE"/plugins/platforms/libqwayland*.so)
FFMPEG=("$FIXTURE"/plugins/multimedia/libffmpegmediaplugin.so)
if [[ ! -f "${XCB[0]}" || ! -f "${FFMPEG[0]}" ]] || (( ${#WAYLAND[@]} == 0 )); then
    echo "FAIL: package must include XCB, Wayland and FFmpeg plugins" >&2
    exit 1
fi
if [[ ! -f "$FIXTURE/plugins/wayland-shell-integration/libxdg-shell.so" ]]; then
    echo "FAIL: package must include the Wayland xdg-shell integration" >&2
    exit 1
fi
for binary in "$FIXTURE/bin/ClipDock" "${XCB[@]}" "${WAYLAND[@]}" "${FFMPEG[@]}" \
    "$FIXTURE/plugins/wayland-shell-integration/libxdg-shell.so"; do
    dependencies="$(ldd "$binary")"
    if [[ "$dependencies" == *"not found"* || ( -n "$SDK" && "$dependencies" == *"$SDK/"* ) ]]; then
        printf 'FAIL: runtime dependencies of %s\n%s\n' "$binary" "$dependencies" >&2
        exit 1
    fi
done
result=0
QT_QPA_PLATFORM=xcb xvfb-run -a -s '-screen 0 1280x720x24' \
    timeout 5s "$FIXTURE/bin/ClipDock" > "$FIXTURE/startup.log" 2>&1 || result=$?
if [[ "$result" != 124 ]]; then
    cat "$FIXTURE/startup.log" >&2
    echo "FAIL: relocated app did not remain running (exit $result)" >&2
    exit 1
fi
echo "PASS: relocated Linux package, XCB/Wayland/FFmpeg dependencies, X11 app startup"
