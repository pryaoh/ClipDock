#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export CLANG_MODULE_CACHE_PATH="$ROOT/macOS/.build/ModuleCache"
export SWIFTPM_MODULECACHE_OVERRIDE="$CLANG_MODULE_CACHE_PATH"
mkdir -p "$CLANG_MODULE_CACHE_PATH"
swift run --package-path "$ROOT/macOS" --disable-sandbox --cache-path "$ROOT/macOS/.build/cache" --config-path "$ROOT/macOS/.build/config" --security-path "$ROOT/macOS/.build/security" --manifest-cache local ScreenEncoderChecks "$@"
