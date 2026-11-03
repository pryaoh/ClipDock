#!/usr/bin/env bash
set -euo pipefail
run_dir=$(mktemp -d)
openbox --sm-disable > "$run_dir/openbox.log" 2>&1 &
wm_pid=$!
cleanup() {
    kill "$wm_pid" 2>/dev/null || true
    wait "$wm_pid" 2>/dev/null || true
    rm -rf -- "$run_dir"
}
trap cleanup EXIT
for attempt in {1..50}; do
    if xprop -root _NET_SUPPORTING_WM_CHECK | grep -q 'window id'; then break; fi
    sleep 0.1
done
"$@"
