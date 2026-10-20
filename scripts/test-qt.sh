#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
"$ROOT/scripts/build-qt.sh" "$@"
ctest --test-dir "$ROOT/build/qt" -C "${CONFIGURATION:-Release}" --output-on-failure
