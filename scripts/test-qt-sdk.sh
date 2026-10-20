#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "$ROOT/scripts/qt-sdk.sh"
FIXTURE="$(mktemp -d "${TMPDIR:-/tmp}/clipdock-sdk.XXXXXX")"
trap 'rm -rf "$FIXTURE"' EXIT
SDK="$FIXTURE/project with spaces/.tools/qt/$CLIPDOCK_QT_VERSION/macos"
mkdir -p "$SDK/bin" "$SDK/plugins/multimedia"
cat > "$SDK/bin/qmake" <<'QMAKE'
#!/bin/bash
[[ "$1" == -query && "$2" == QT_INSTALL_PLUGINS ]] || exit 1
printf '%s/plugins\n' "$(cd "$(dirname "$0")/.." && pwd)"
QMAKE
chmod +x "$SDK/bin/qmake"
if qt_sdk_has_ffmpeg "$SDK"; then
    echo "FAIL: Qt without the FFmpeg backend must be rejected" >&2
    exit 1
fi
touch "$SDK/plugins/multimedia/libffmpegmediaplugin.dylib"
qt_sdk_has_ffmpeg "$SDK"
# Detection must still use the installed plugin if qmake cannot run in a sandbox.
printf '#!/bin/bash\nexit 1\n' > "$SDK/bin/qmake"
qt_sdk_has_ffmpeg "$SDK"
unset QT_ROOT
[[ "$(qt_sdk_select "$FIXTURE/project with spaces")" == "$SDK" ]]
export QT_ROOT="$FIXTURE/explicit SDK"
[[ "$(qt_sdk_select "$FIXTURE/project with spaces")" == "$QT_ROOT" ]]
if qt_sdk_has_ffmpeg "$QT_ROOT"; then
    echo "FAIL: missing SDK must be rejected" >&2
    exit 1
fi
echo "PASS missing FFmpeg detection, local SDK selection, paths with spaces, explicit QT_ROOT"
