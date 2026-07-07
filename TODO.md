# Candle TODO

Updated: 2026-07-07

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

## Remaining

- Manually launch the Standalone app and verify:
  - Audio passes cleanly with no silence, NaNs, or garbage output.
  - Saturation sounds smooth and behaves as expected.
  - Bypass toggling while audio is playing does not cause silence or stuck state.
  - Double-click resets all eight knobs to defaults.
  - Save, rename, and delete user preset flows work from the UI.
  - Renaming a user preset to its current name does not delete the preset file.
- Verify in REAPER:
  - VST3 audio output remains correct.
  - Plugin latency/PDC appears sane.
  - No obvious sync drift from oversampling latency reporting.
- Optionally inspect or log the actual oversampling latency at common rates before documenting a numeric value.
- Consider a future CPU optimization only if profiling shows the full-tape 2x oversampling path is too expensive.
- Consider adding an automated plugin smoke-test or validation script if this workflow will repeat often.
