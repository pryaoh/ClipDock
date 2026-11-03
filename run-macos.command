#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
"$ROOT/scripts/build-qt.sh"
open "${BUILD_DIR:-$ROOT/build/qt}/ClipDock.app"
