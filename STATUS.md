# Candle Status

Updated: 2026-07-07

## Built This Session

- Added 2x FIR oversampling around the cassette tape processing path.
  - `MicStage` remains at native sample rate.
  - `CassetteTape` is prepared and processed at the oversampled rate.
  - Plugin latency is reported from JUCE oversampling with integer latency enabled.
  - Oversampling state is reset wherever tape state is reset after bypass or sanitization.
- Added explicit double-click-to-reset behavior for all shared `CandleKnob` controls.
- Added user preset management.
  - User presets can now be deleted.
  - User presets can now be renamed.
  - Factory presets remain protected from delete/rename actions.
  - The preset UI stays in-editor and does not use detached/native dialogs.
- Added a Standalone Plugin target to the JUCE project.
  - `Candle.jucer` now builds AU, VST3, and Standalone formats.
  - Projucer regenerated the Xcode project and standalone plist.

## Verification Completed

- Re-saved the project with:
  - `/Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave Candle.jucer`
- Built all Release targets with:
  - `xcodebuild -project Candle.xcodeproj -configuration Release -alltargets build`
- Confirmed build success for:
  - AU component
  - VST3 bundle
  - Standalone app
  - Shared code
  - VST3 manifest helper
- Ran AU validation:
  - `auval -v aufx CNDL RUBS`
  - Result: `AU VALIDATION SUCCEEDED`
  - Render tests passed across multiple sample rates and block sizes.
  - Latency property check passed.
- Installed the Release artifacts into systemwide locations:
  - `/Library/Audio/Plug-Ins/Components/Candle.component`
  - `/Library/Audio/Plug-Ins/VST3/Candle.vst3`
  - `/Applications/Candle.app`
- Verified code signatures for all three systemwide installs.

## Build Artifacts

- Release AU: `Builds/MacOSX/build/Release/Candle.component`
- Release VST3: `Builds/MacOSX/build/Release/Candle.vst3`
- Release Standalone: `Builds/MacOSX/build/Release/Candle.app`

## Notes

- The full Release build installed AU and VST3 into the current user's plugin folders as part of the existing Xcode install phases.
- The same Release AU, VST3, and Standalone app were also copied into systemwide folders after the build.
- Manual GUI and DAW smoke testing is still recommended before shipping, especially Standalone interaction, user preset rename/delete behavior, bypass transitions, and REAPER latency/PDC behavior.
