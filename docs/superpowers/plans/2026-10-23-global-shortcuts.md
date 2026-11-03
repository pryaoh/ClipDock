# Global shortcuts implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Issue #1: control recording while another application has focus, retaining safe app-local fallback.

**Architecture:** A shared Qt shortcut manager chooses exactly one route per command: registered system shortcut or non-repeating QShortcut. Platform backends report registration results and activations; MainWindow dispatches through existing enabled controls with modal/picker/start-request guards.

**Tech Stack:** Qt 6.8/C++20, Windows RegisterHotKey, macOS Carbon hot-key events, X11 passive key grabs, Wayland GlobalShortcuts portal via Qt DBus.

**Spec:** https://github.com/pryaoh/ClipDock/issues/1 and the user's instruction to implement on a separate branch.

## Constraints and decisions

- Work on `feat/global-shortcuts`; keep main unchanged.
- Keep Ctrl+Shift+R/P/S; Qt's Ctrl mapping means Command on macOS.
- Global start opens the existing save dialog; automatic file naming/presets remain issue #2.
- Report failures per command, permit retry, retain local shortcuts on unsupported platforms or rejected permission requests.
- Do not deliver actions during modal dialogs, source selection, duplicate save requests, or invalid recording states.
- Release OS registrations and portal sessions on destruction/retry. Suppress held-key repeats and duplicate local delivery.

## Review focus

- Partial registration conflict must not disable unaffected commands or release another application's registration.
- Native callbacks must not enter capture/save dialogs inside an OS event handler.
- Late portal replies after retry/closure must not reactivate stale bindings.
- Keyboard lock modifiers/repeats must not change the requested action.
- Saving, picking a region, permission waiting and finalizing files must remain safe under repeated shortcut input.

## Tasks

### 1. Common manager and safe command routing
Files: `qt/src/GlobalShortcuts.*`, `qt/src/ShortcutBackend.h`, `qt/src/MainWindow.*`, `qt/tests/GlobalShortcutsTests.cpp`, `qt/tests/MainWindowTests.cpp`, `CMakeLists.txt`.
- [x] Demonstrate existing repeating QShortcuts fail a regression assertion.
- [x] Add `ShortcutAction { Record, Pause, Stop }` and backend registration/activation signals.
- [x] Implement per-command local fallback, status text, retry and queued native activation.
- [x] Route through enabled buttons with modal/picker guards; guard `begin()` against reentrancy.
- [x] Test fallback, partial success, repeat suppression and invalid-state/modal dispatch.

### 2. Native desktop registration
Files: `qt/src/NativeGlobalShortcuts.cpp`, `qt/tests/NativeGlobalShortcutsTests.cpp`, build dependencies.
- [x] Add actual OS registration/conflict/release and event-delivery tests.
- [x] Implement message-only Windows registration with MOD_NOREPEAT; Carbon press/release handling; X11 lock-modifier grabs and repeat suppression.
- [x] Run Windows native tests and Linux/Xvfb checks where available; document macOS execution limitations.

### 3. Wayland portal
Files: `qt/src/PortalGlobalShortcuts.*`, `qt/tests/PortalGlobalShortcutsTests.cpp`, Linux DBus linkage.
- [x] Add controlled D-Bus service tests for accept/reject, partial bindings, repeat activation and stale responses.
- [x] Implement async capability check, session creation/binding, activation/deactivation and closure.
- [x] Reflect portal-selected triggers and service/session disappearance; never claim unsupported environments are registered.

### 4. Verification and documentation
Files: `docs/usage.md`, `docs/testing.md`, `docs/build-linux.md` if dependency instructions require updates.
- [x] Build Windows Release and run the complete suite; run Linux checks available in WSL.
- [x] Review native registration lifetime and command routing; resolve substantive findings.
- [x] Update runtime package and document keys, status/retry, limitations, verified platforms.
- [x] Leave changes on the feature branch, without merging main.

## Verification result (2026-10-26)

- Windows Release: 14/14 CTest checks passed; runtime package refreshed.
- Linux Release under WSL/Xvfb: 16/16 CTest checks passed, including foreign-focus XTest input and isolated portal protocol tests.
- Compact 920×620 and 150% DPI UI renders inspected; shortcut status remains visible.
- Review fixes: preserve queued X11 events across mapping notifications; reject overlapping/unmodified secondary keys; forward portal activation tokens only with their queued action; resolve macOS letters from the shortcut keyboard layout.
- Actual Windows/macOS physical keys and real Wayland compositor approval/focus behavior require desktop checks. macOS compilation and US/Dvorak test are delegated to the branch's CI runners.
