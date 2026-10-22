#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
bash "$ROOT/scripts/build-qt.sh" "$@"
DEFAULT_BUILD="$ROOT/build/qt"
if [[ "$(uname -s)" == Linux ]]; then DEFAULT_BUILD="$ROOT/build/qt-linux"; fi
ctest --test-dir "${BUILD_DIR:-$DEFAULT_BUILD}" -C "${CONFIGURATION:-Release}" --output-on-failure
