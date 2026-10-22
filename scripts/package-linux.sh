#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != Linux ]]; then
    echo "Linux에서 실행하세요." >&2
    exit 1
fi
bash "$ROOT/scripts/build-qt.sh" "$@"
cmake --install "${BUILD_DIR:-$ROOT/build/qt-linux}" \
    --config "${CONFIGURATION:-Release}" --prefix "$ROOT/dist/linux"
echo "Linux 패키지: $ROOT/dist/linux/bin/ClipDock"
