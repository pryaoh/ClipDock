#!/usr/bin/env bash
# Shared SDK selection. Sourcing this file does not install or modify anything.
CLIPDOCK_QT_VERSION=6.8.3

qt_sdk_has_ffmpeg() {
    local qt_sdk_root="$1" qt_sdk_plugins
    [[ -x "$qt_sdk_root/bin/qmake" ]] || return 1
    # Official kits and Homebrew use these layouts. Prefer file inspection:
    # qmake's CPU probe can fail under a restricted runner even for a valid kit.
    local qt_sdk_directory
    for qt_sdk_directory in "$qt_sdk_root/plugins/multimedia" "$qt_sdk_root/share/qt/plugins/multimedia"; do
        local qt_sdk_files=("$qt_sdk_directory"/*ffmpeg*)
        if [[ -f "${qt_sdk_files[0]}" ]]; then return 0; fi
    done
    qt_sdk_plugins="$("$qt_sdk_root/bin/qmake" -query QT_INSTALL_PLUGINS 2>/dev/null)" || return 1
    [[ -d "$qt_sdk_plugins/multimedia" ]] || return 1
    local qt_sdk_matches=("$qt_sdk_plugins"/multimedia/*ffmpeg*)
    [[ -f "${qt_sdk_matches[0]}" ]]
}

qt_sdk_select() {
    local project_root="$1" local_sdk
    if [[ -n "${QT_ROOT:-}" ]]; then
        printf '%s\n' "$QT_ROOT"
        return
    fi
    case "$(uname -s)" in
        Darwin) local_sdk="$project_root/.tools/qt/$CLIPDOCK_QT_VERSION/macos" ;;
        Linux) local_sdk="$project_root/.tools/qt/$CLIPDOCK_QT_VERSION/gcc_64" ;;
        *) return 0 ;;
    esac
    if qt_sdk_has_ffmpeg "$local_sdk"; then
        printf '%s\n' "$local_sdk"
    elif [[ "$(uname -s)" == Darwin ]] && command -v brew >/dev/null 2>&1; then
        brew --prefix qt 2>/dev/null || true
    fi
}
