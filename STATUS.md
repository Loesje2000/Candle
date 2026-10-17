# Candle Status

Updated: 2026-10-17 (v0.1.2)

## This Session (v0.1.2)

- Renamed the SM57 mic model to just "57" in the plugin UI and scrubbed the
  brand name from source comments/tooltips (legal reasons). Saved states are
  unaffected (choice params store the index, not the label).
- Version bumped 0.1.1 → 0.1.2, project re-saved, all Release targets rebuilt
  (0 errors), `auval -v aufx CNDL RUBS` PASSED.
- Re-installed Release builds via `ditto` (AU + VST3 in `~/Library/Audio/Plug-Ins/`,
  Standalone in `/Applications`), all verified 0.1.2 and signed.
- REAPER plugin caches cleared again before install (backups:
  `.candle-cache-clean-20261017-214939`).

## v0.1.1 (same day)

- Version bumped 0.1.0 → 0.1.1 in `Candle.jucer`; project re-saved with Projucer 8.0.14
  (also bumped `JUCE_PROJUCER_VERSION` 0x8000d → 0x8000e, fixing the build against the
  installed JUCE 8.0.14).
- Rebuilt all Release targets successfully; AU validation (`auval -v aufx CNDL RUBS`) PASSED.
- Installed Release builds:
  - AU: `~/Library/Audio/Plug-Ins/Components/Candle.component` (0.1.1, signed, universal)
  - VST3: `~/Library/Audio/Plug-Ins/VST3/Candle.vst3` (0.1.1, signed, universal)
  - Standalone: `/Applications/Candle.app` (0.1.1, signed, universal)
  - NOTE: use `ditto`, not `cp -R`, for bundle installs — `cp -R` silently produced a
    partial copy (missing Info.plist) this session.
- Cleared REAPER plugin caches before install (timestamped backups kept in the REAPER
  folder, pattern `.candle-cache-clean-20261017-182704`):
  `reaper-auplugins_arm64.ini`, `reaper-vstplugins_arm64.ini`. REAPER will rescan on
  next launch.
- MicStage: retuned the Realistic model (was piercingly bright — see
  `docs/research-notes.md` §1) and added an SM57 model behind a new `mic_model`
  choice parameter (OFF / REALISTIC / SM57, default REALISTIC). The old `mic_on`
  bool parameter was removed — old sessions/presets will drop it.
- Modeling provenance audit: nothing in the DSP was measured; all values are by-ear
  estimates. Research notes + next steps are in `docs/research-notes.md`.

## Previous Session (2026-07-09)

- Reworked the GUI to a warmer "BuckleyHaze"-style chassis theme (`CandleLookAndFeel.h/.cpp`, `CandleKnob.cpp`):
  - New amber/cream/warm-brown colour palette.
  - Rotary knobs redrawn with a gradient body, gloss highlight, rim shadow, and glowing value arc instead of the old flat ring.
  - Buttons, toggles, and combo boxes redrawn with amber-glow "on" states and auto-fitting label text.
  - Panel section titles ("MIXER", "TAPE", "OUTPUT") now render as an amber plate broken into the panel's rule line.
  - Header, badge, and cassette-window graphics repositioned/restyled to match.
- Fixed a layout bug from that redesign where the new panel-title plates visually collided with the TRIM knob (MIXER panel) and the OUTPUT knob (OUTPUT panel) — children paint on top of the panel title, so the knobs were sitting directly over the lettering. Moved both knobs down and added a small clearance margin under the TAPE panel's DBX/SPEED/GEN row (`PluginEditor::resized()`).
- Fixed mojibake ("weird letters") in tooltip text (`Input trim — set ... (±20 dB)` rendering as `Input trim â€" set ... (Â±20 dB)`). Root cause: `juce::String(const char*)` does not assume UTF-8, so multi-byte characters like em dash (—) and plus-minus (±) in the source literals were being split into garbage glyphs at render time. Replaced all non-ASCII punctuation in `setTooltip(...)` calls with plain ASCII (`-`, `+/-`, `3-3/4`).
- Redesigned the VU meter (`drawVUMeter` in `PluginEditor.cpp`) to look like a real Portastudio-style analog meter instead of a flat tick row:
  - Tick marks, numbers, and the needle now share a single pivot point below the visible glass and the same angle mapping, so the needle actually points at the correct tick.
  - Scale reads `- 20 10 7 5 3 1 [0] 3 +`, matching the reference hardware photo.
  - Added a thin scale arc across the top of the face (black for the normal range, red from 0 to +3) and a red "3" tick to mark the danger zone.
  - Face now has subtle radial shading instead of a flat fill; "VU" label is larger and bold italic.

## Verification Completed

- Rebuilt the Debug Standalone target after each change via:
  - `xcodebuild -project Builds/MacOSX/Candle.xcodeproj -scheme "Candle - Standalone Plugin" -configuration Debug build`
  - All builds succeeded.
- The panel-title/knob overlap fix and tooltip text fix were visually confirmed by the user running the Standalone build.
- The VU meter redesign was **not** visually confirmed this session — Terminal in this environment lacks macOS Screen Recording permission, so automated screenshots only captured the desktop wallpaper, not the app window. The change was verified by re-deriving the geometry (pivot, angle range, tick radii) by hand and confirming it builds; a human look in the running app is still needed.

## Build Artifacts

- Debug Standalone (used for this session's manual checks): `Builds/MacOSX/build/Debug/Candle.app`
- Release AU: `Builds/MacOSX/build/Release/Candle.component`
- Release VST3: `Builds/MacOSX/build/Release/Candle.vst3`
- Release Standalone: `Builds/MacOSX/build/Release/Candle.app`

## Notes

- No Release build was produced this session — only Debug Standalone, for GUI iteration. Re-run the Release build/install/AU-validation steps from the previous session before shipping these GUI changes.
- The prior session's outstanding manual QA (audio behaviour, preset flows, REAPER latency/PDC) has not been re-verified and still applies — see `TODO.md`.
