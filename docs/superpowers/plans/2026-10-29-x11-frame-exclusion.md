# X11 self-capture frame exclusion

Goal: prevent recorder pixels in saved X11 screen/region recordings during start,
restore and resume, while preserving preview and the exclusion opt-out.

Scope: continue the existing feature branch, preserving the three pending documentation
edits. The user requested this specific fix; implement and verify inline.

Root cause: QScreenCapture feeds QMediaRecorder independently of UI minimization.
Already captured frames survive the state change, and restoration can be captured before
the queued UI pause. Waiting a fixed duration or discarding a fixed frame count cannot
establish which frame is safe.

Approach: for protected X11 screen/region recording, acquire fresh frames synchronously
on Qt's X11 connection, checking native excluded-window visibility and overlap under
the same short server grab as pixel acquisition. Feed only safe frames through the
existing QVideoFrameInput path. Keep preview and other platforms on the current path.
Release the server before any UI signal, image processing or encoding. Block rather
than fall back to unchecked capture on query failures.

Additional observed cause: after native unmap, the same-process background can still
contain recorder pixels until Qt handles its expose/repaint events. At start/resume
and after a blocked acquisition, observe native nonoverlap, release the server,
synchronize Qt window updates, revalidate the live session and reacquire. This is
not a repaint fence for external applications or compositing window managers.

- [x] Promote the real X11/Openbox recording/decode reproducer to a regression test;
      require zero marked recorder frames throughout the movie and observe failure.
- [x] Add protected X11 acquisition, integrate start/resume/full-screen/region timestamps,
      preserve pause duration, stop and error cleanup, and exclusion opt-out.
- [x] Run regression cases (preview/direct start, region, opt-out), Linux full CTest,
      Windows full CTest and diff checks. Review the final change.
- [x] Update usage/testing/Todo with exact evidence and remaining environment limits.

Review focus: stale preview/trigger frame entering encoder; restore before UI events;
region coordinates and scaling; no ungrabbed error path; end/pause/restart lifecycle;
screen changes, lost excluded window, non-X11 and exclusion-disabled behavior.

Evidence: original build reproduced 2 recorder frames in
`.cache/wsl-desktop.E5jL7x/integration.log`. Native visibility gating alone removed
startup contamination but retained 1 post-resume frame; diagnostics showed unmapped
window plus stale magenta background. Qt synchronization removed that frame in the
direct-start regression. Independent review found a fractional-DPI endpoint mismatch;
the new native regression failed before using the exact `regionPixelRect` endpoints.
Review of the final implementation found no further important issue.

Final validation: Linux Release CTest 23/23 and Windows Release CTest 17/17 passed.
Protected preview/direct/region recordings decoded 28/31/30 background frames with
zero recorder-marker frames; opt-out control decoded 75 marker frames. Native 125%
edge, nonoverlap and error cleanup checks passed. `git diff --check` passed.
