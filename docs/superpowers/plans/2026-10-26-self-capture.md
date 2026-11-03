# Self Capture Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans inline with one final independent review.

**Goal:** Prevent recursive self capture with opt-out exclusion and optional automatic minimization.
**Architecture:** MainWindow owns persistent UI preferences, native window exclusion and preview visibility. RecorderController exposes its selected target for overlap decisions; recording/media lifecycle stays unchanged.
**Tech Stack:** Qt 6.8, C++20, QSettings, Windows display affinity.
**Spec:** ../specs/2026-10-26-self-capture-design.md

## Constraints
- Preserve the current feature branch and existing changes. No new dependencies or capture backend.
- Defaults: exclusion true, minimization false. Exclusion active only outside Idle.
- Unsupported platforms must retain a reachable pause/stop UI.

## Review Focus
- Settings isolation in tests and persistence across window instances.
- Native HWND recreation, owned dialogs and restoring WDA_NONE after stop.
- Capture region overlap, monitor movement, unknown portal target.
- Permission/start failures and manual versus app-driven minimization.
- Resume and close while fallback minimization is active.

## Task 1: Preferences and capture UI policy
Files: qt/src/MainWindow.{h,cpp}, qt/src/RecorderController.{h,cpp}, qt/src/UiTheme.h, qt/resources/checkmark.xpm, qt/tests/MainWindowTests.cpp, qt/tests/RegionRecordingTests.cpp, CMakeLists.txt.
- [x] Add failing default/persistence UI test; run main_window and confirm missing options.
- [x] Add named checkboxes and scoped settings storage; expose `const Target &target() const`.
- [x] Implement exclusion application/reset, overlap preview guidance, minimize/restore/pause transitions.
- [x] Test offscreen overlap and lifecycle, native affinity default/off/active/idle and owned dialogs, conditional older Windows fallback.
- [x] Run full Windows CTest, real screen pixel test and independent review; fix important findings. Final Windows suite: 17/17 passed. Minimize at `recordingAboutToStart`, after source/output readiness; pause on restored/moved/resized overlap. Actual older Windows/macOS/Wayland desktop validation remains separate.

## Task 2: Delivery
Files: docs/usage.md, docs/testing.md, docs/Todo.md, local dist.
- [x] Document defaults, scope and platform fallback; keep #2/#5 incomplete.
- [x] Package separately from running app, verify packaged startup and report actual test scope.
