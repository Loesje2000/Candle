# Candle — Modeling Provenance & Research Notes

Updated: 2026-10-17

## 1. Provenance audit — where the current numbers came from

**Nothing in the current DSP was measured or sourced from documentation.** Both
stages were written as by-ear estimates:

### MicStage ("REALISTIC" button)
- Arrived in commit `bb9db5e` ("Add full source"), tuned once in `4c6571b`
  ("Harden audio processing safety"), retuned by ear 2026-10-17 (presence peak
  3.2 kHz +4 dB Q1.4, LPF 6.5 kHz, dulling LPF 10 kHz) because the original
  wide +6 dB @ 3.8 kHz blast made the stage piercingly bright.
- No measurement of a real Realistic mic, no datasheet, no SPICE. The comment
  in `Source/dsp/MicStage.h` now states this explicitly.

### CassetteTape stage
- Also from `bb9db5e`; all values are generic Portastudio-flavored estimates:
  - IEC Type I shelf: ±6 dB @ 1326 Hz (120 µs) — textbook IEC I, not unit-specific
  - Speed bandwidths (HIGH 16k / NORM 14k / SLOW 9.5 kHz) — invented
  - Head bump ~80 Hz, generation-loss 1.35×/gen, dropout/noise/hum params — invented
- The header says "Tascam Porta 424" and `.jucer` pluginDesc says "Porta 424
  cassette processor" — a nominal label only. No 424 (or Porta One) manual was
  consulted.

## 2. What the PortaOne project already provides

`~/Documents/New project/PortaOne/` (user's physical Porta One, JUCE project):

- `docs/manual-findings.md` — extracted from the **actual TASCAM Porta One
  service + user manuals** (`~/Downloads/tascam-porta-one-*.pdf`):
  - Type II, 70 µs; single speed 4.8 cm/s; ±15 % pitch
  - 3180 µs + 70 µs EQ; 160 nWb/m = 0 VU; −10 dBV nominal
  - Overall response 40 Hz–12.5 kHz ±3 dB; 1 % THD @ 0 VU
  - S/N ~54 dB (no dbx) / ~80 dB (dbx); 50 dB channel separation
  - Mixer EQ: 100 Hz / 10 kHz shelves ±10 dB — **same values Candle guesses at**
- `spice/porta_one_manual_target.cir` + `.log` — ngspice AC sweep meeting the
  manual's bandwidth endpoints (−3 dB @ 40 Hz, −3.1 dB @ 12.5 kHz confirmed)
- `spice/porta_one_mixer_eq.cir` — shelving EQ sketch (needs pot topology trace
  from the service schematic)
- `calibration/` — measurement WAV generator, cue sheet, and full recording
  checklist for sweeping the physical unit (flat / EQ extremes / dbx on-off /
  driven)

**These are directly reusable in Candle.** The manual targets should replace
Candle's invented tape values; the SPICE methodology (AC-validate analog
assumptions offline, then fit stable IIRs — never run SPICE in real time) is
already documented there.

## 3. Web research log (2026-10-17)

Automated search was attempted via Google, DuckDuckGo (html + lite), Bing,
Mojeek, Ecosia, SearXNG, and Equipboard — **all returned captchas, 403s, or
garbage results** for automated fetches. Direct fetches that succeeded:

- Wikipedia `Either/Or (album)` (raw wikitext, Recording section): recording
  locations only — Joanna Bolme's house, Smith's house, Undercover Inc.,
  Laundry Rules Recording, Heatmiser House (all Portland OR), The Shop
  (Arcata CA). **No recorder/mic model named.**
- sweetadeline.net FAQ: lists guitars/amps/pedals only; points to
  newmonkeystudio.com for studio gear. No Portastudio model.

**Unverified (from memory, needs manual confirmation):** the TASCAM Portastudio
model Elliott Smith used for Roman Candle / Elliott Smith / Either/Or is widely
described in interviews and biographies as a 4-track Portastudio; the Porta One
is the model most often cited by fans, but I could not verify the exact model
number today. Check these (human-readable, likely to work in a browser):
  - *Torment Saint: The Life of Elliott Smith* (William Todd Schultz)
  - Tape Op interviews with Larry Crane, and his 2013 Either/Or reissue notes
  - Either/Or expanded-edition liner notes (Kill Rock Stars, 2013)
  - *Heaven Adores You* (2014 documentary)
  - newmonkeystudio.com equipment list (his later studio — analog, not 4-track)

## 4. Methodology recommendation (SPICE vs. measurement)

SPICE is the right tool **only for the electrical stages**; it is the wrong
tool for the parts that actually create the character of the mic and tape:

| Element | SPICE useful? | Better source |
|---|---|---|
| Porta One mic pre / line amps / mixer EQ / dbx chip I/O | **Yes** — trace the service-manual schematic, run AC sweeps, fit biquads + mild nonlinearity | Service manual (already on disk) |
| Realistic 33-984 mic | **Mostly no** — the electrical side is just a voice coil (trivial). The character is *acoustic* (capsule resonance, grill, proximity) | Measure a real unit: buy a 33-984 (~$25–40 on eBay/Reverb), sine-sweep with REW against a measurement mic, fit MicStage filters to the measured curve. Fallback: published catalog response curve |
| Tape head / cassette nonlinearity | **No** — transient SPICE of magnetics is too slow for real time | Measured transfer curves from the calibration checklist; hysteresis-style DSP (e.g., Jiles–Atherton-derived) for saturation |
| Record/playback EQ (3180+70 µs) | Yes (validates corner frequencies) | IEC I/II standards + manual |

## 5. Proposed next steps (priority order)

1. **Confirm which Portastudio Elliott Smith used** (sources in §3). If it is
   the Porta One — and you own one — retarget Candle to the Porta One.
2. **Port the PortaOne manual targets into Candle** (response 40 Hz–12.5 kHz,
   EQ corners 100 Hz/10 kHz, dbx S/N gap, single-speed + pitch instead of the
   invented HIGH/NORM/SLOW trio). This kills most invented numbers.
3. **Run the PortaOne calibration checklist** on the physical unit and fit
   Candle's filters/saturation to the recorded sweeps.
4. **Buy a Realistic 33-984** and measure it the same way; fit MicStage to the
   measured curve. (Also fixes the electret-vs-dynamic question in the comments.)
5. **SPICE the Porta One electrical chain** from the service schematic (mic
   pre, EQ, dbx I/O), convert AC results to filter coefficients — extend
   `PortaOne/spice/`.

## 6. "57" mic model added (2026-10-17)

Roman Candle sessions also used a classic industry-standard dynamic mic (named
simply "57" in the plugin UI for legal reasons), so the mic stage now has two
models behind a `mic_model` choice parameter (OFF / REALISTIC / 57):

- `MicStage::Parameters` gained `model` (0 = Realistic, 1 = 57); the four
  IIR banks swap coefficients on model change.
- 57 curve (from the widely published response, still an approximation):
  −3 dB low shelf @ 160 Hz, +5.5 dB broad peak @ 8.2 kHz (Q 0.9),
  2nd-order LPF @ 12 kHz + 1st-order @ 16 kHz, self-noise ~−74 dBFS.
- UI: the REALISTIC toggle became a MIC combo on the MIXER panel.
- Default is REALISTIC (index 1); the "Candle" factory preset sets it too.
- Old `mic_on` bool parameter is gone — saved presets/sessions referencing it
  will drop that value (pre-release, accepted).

