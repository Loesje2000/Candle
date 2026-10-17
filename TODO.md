# Candle TODO

Updated: 2026-10-17

## Completed

- Fixed the earlier VST3 silent-output issue in REAPER by removing the legacy fixed channel config macro conflict.
- Added tooltip toggle, preset navigation, preset saving, and product naming updates.
- Added 2x FIR oversampling around `CassetteTape::process`.
- Paired oversampling resets with tape resets for bypass and audio sanitization paths.
- Added explicit double-click reset-to-default behavior for knobs.
- Added delete and rename support for user presets.
- Added the Standalone Plugin target.
- Re-saved the JUCE project with Projucer.
- Built all Release targets successfully.
- Ran AU validation successfully.
- Installed AU, VST3, and Standalone app into systemwide folders.
- Verified systemwide bundle code signatures.
- Reworked the GUI to the "BuckleyHaze" warm amber/cream theme (knobs, buttons, toggles, combo boxes, panel titles, header/badge).
- Fixed the panel-title vs. knob overlap introduced by that redesign (MIXER/TRIM and OUTPUT/OUTPUT knob) — confirmed fixed by the user.
- Fixed mojibake in tooltip strings (em dash / plus-minus rendering as garbage characters) by removing non-ASCII punctuation from tooltip text.
- Redesigned the VU meter to a proper pivoting-arc analog meter (shared pivot for ticks/labels/needle, red danger-zone arc) — not yet visually confirmed.
- Fixed the build against installed JUCE 8.0.14 (re-saved project, `JUCE_PROJUCER_VERSION` 0x8000d → 0x8000e).
- Retuned the Realistic mic stage (was piercingly bright: wide +6 dB @ 3.8 kHz, gentle 9 kHz LPF → narrow +4 dB @ 3.2 kHz, 6.5 kHz cliff) — see `docs/research-notes.md`.
- Added a second mic model ("57", classic dynamic) behind a new `mic_model` choice parameter (OFF / REALISTIC / 57); replaced the REALISTIC toggle with a MIC combo. Old `mic_on` bool parameter removed.
- Modeling provenance audit: documented that all DSP values are by-ear estimates; research plan and next steps in `docs/research-notes.md`.
- Bumped version to 0.1.2; re-ran full Release build (all targets, 0 errors) and AU validation (PASS); re-installed AU/VST3/Standalone and cleared the REAPER plugin caches (timestamped backups kept) before install.

## Remaining

- Visually confirm the new VU meter in the running app (this session's automated screenshot attempts failed — no Screen Recording permission in this environment) and tune the arc/angle/spacing if it doesn't look right.
- Audition the retuned Realistic model and the new 57 model by ear (especially how the 57's 8.2 kHz edge sits through the tape stage) and fine-tune peak gain / LPF corner if needed.
- Manually launch the Standalone app and verify:
  - Audio passes cleanly with no silence, NaNs, or garbage output.
  - Saturation sounds smooth and behaves as expected.
  - Bypass toggling while audio is playing does not cause silence or stuck state.
  - Double-click resets all eight knobs to defaults.
  - Save, rename, and delete user preset flows work from the UI.
  - Renaming a user preset to its current name does not delete the preset file.
- Verify in REAPER (fresh rescan after cache clear):
  - VST3/AU audio output remains correct.
  - Plugin latency/PDC appears sane.
  - No obvious sync drift from oversampling latency reporting.
  - Mic model parameter automates correctly.
- Confirm which Portastudio model Elliott Smith actually used (Porta One vs other), then port the Porta One manual targets from `~/Documents/New project/PortaOne` (manual findings + SPICE + calibration checklist) into Candle — see `docs/research-notes.md` §5.
- Measure a real Realistic mic (and optionally a 57) with a sine sweep and fit the mic stage filters to the measurements.
- Optionally inspect or log the actual oversampling latency at common rates before documenting a numeric value.
- Consider a future CPU optimization only if profiling shows the full-tape 2x oversampling path is too expensive.
- Consider adding an automated plugin smoke-test or validation script if this workflow will repeat often.
