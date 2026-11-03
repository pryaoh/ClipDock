#!/usr/bin/env bash
set -euo pipefail
APP="${1:?Usage: clean-macos-signing-metadata.sh /path/to/ClipDock.app}"
[[ -d "$APP/Contents" ]] || { echo "앱 번들이 아닙니다: $APP" >&2; exit 1; }
# codesign rejects these legacy Finder attributes, including on nested bundles.
# Preserve all other attributes (in particular quarantine and provenance).
/usr/bin/xattr -dr com.apple.FinderInfo "$APP"
/usr/bin/xattr -dr com.apple.ResourceFork "$APP"
