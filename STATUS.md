# Candle Status

Updated: 2026-07-09

## Built This Session

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
