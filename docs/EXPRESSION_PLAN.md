# Expression & Macro-Gesture Integration Plan

## Overview

Extending the plugin with 5 DSP modules and a macro-control architecture driven by hand gesture (index fingertip Y position). Development is strictly sequential — each step must build and pass audio tests before the next begins.

**Decisions locked in:**
- Macro control uses **Option B (modulation depth)**: individual knobs stay active as baseline; Y gesture adds an offset scaled by a per-parameter Y Amount knob
- Gesture tracking uses **index fingertip** (landmark 8)
- A **toggle button** switches between Classic mode (existing up/down → dryWet) and Y-Track mode (index tip Y → macroY)

---

## Signal Routing (target)

```
Input → GainModule → Saturation → TimbreFilter → Vibrato
                                                      ↓
                                             dryWetMixer.saveDry()
                                                      ↓
                        [existing: InputFilter → Predelay → ER → FDN → ... → mixToOutput]
```

---

## Parameter Reference (21 new, total 52)

### Module 1 — Gain
| ID | Range | Default |
|----|-------|---------|
| `gainDb` | −24 to +12 dB | 0 dB |
| `outputTrimDb` | −12 to 0 dB | 0 dB |

### Module 2 — Timbre
| ID | Range | Default |
|----|-------|---------|
| `timbreCutoff` | 20–20 000 Hz (skew 0.3) | 8 000 Hz |
| `timbreResonance` | 0.5–10 (skew 0.5) | 0.7071 |
| `timbreDrivePre` | 0–12 dB | 0 dB |

### Module 3 — Saturation
| ID | Range | Default |
|----|-------|---------|
| `satDrive` | 0–1 | 0 |
| `satMix` | 0–100 % | 0 % |
| `satTone` | 0–1 | 1 (bright) |

### Module 5 — Modulation (Vibrato)
| ID | Range | Default |
|----|-------|---------|
| `vibratoRate` | 0.1–8 Hz (skew 0.5) | 2 Hz |
| `vibratoDepth` | 0–1 | 0 |
| `vibratoFadeIn` | 0–500 ms (skew 0.5) | 100 ms |

### Macro Y
| ID | Range | Default |
|----|-------|---------|
| `macroY` | 0–1 | 0 |

### Y Amount (modulation depth per parameter)
All range −1 to +1, default 0. At 0 = gesture has no effect.

| ID | Targets |
|----|---------|
| `gainYAmt` | gainDb |
| `cutoffYAmt` | timbreCutoff |
| `satDriveYAmt` | satDrive |
| `satMixYAmt` | satMix |
| `vibratoDepthYAmt` | vibratoDepth |
| `dryWetYAmt` | dryWet (existing) |
| `decayYAmt` | decay (existing) |
| `dampingYAmt` | damping (existing) |

### Gesture Mode Toggle
| ID | Type | Default |
|----|------|---------|
| `gestureModeToggle` | Bool | false (Classic) |

---

## Step-by-Step Plan

### ✅ Step 0 — Parameters + Signal Routing Fix
**Status: DONE**

Files changed:
- `Source/Parameters/ParameterIDs.h` — 21 new IDs
- `Source/Parameters/ParameterLayout.h` — 21 new entries with ranges/defaults
- `Source/Plugin/PluginProcessor.cpp` — `saveDry()` moved after new module slot comment
- `WebUI/src/types/parameters.ts` — 21 IDs added to `ParameterId` union

Test: plugin loads, audio unchanged, 21 params dormant but registered.

---

### Step 1 — GainModule
**New file:** `Source/DSP/Input/GainModule.h` (header-only)

DSP design:
- Two `SmoothedValue<float>` (10 ms ramp): one for gainDb→linear, one for outputTrimDb→linear
- Per-sample multiply: `sample *= smoothGain.getNextValue() * smoothTrim.getNextValue()`

Files to change:
- `Source/DSP/Input/GainModule.h` — create
- `Source/Plugin/PluginProcessor.h` — add `GainModule gainModule;` + `smoothGainDb`, `smoothOutputTrimDb`
- `Source/Plugin/PluginProcessor.cpp` — prepare, wire params, insert `gainModule.process(buffer)` in module slot
- `Source/Plugin/PluginEditor.cpp` — add `gainDb`, `outputTrimDb` to timer sync
- `CMakeLists.txt` — add `GainModule.h` to sources
- `WebUI/src/state/store.ts` — add stores: `gainDb: writable(0.667)`, `outputTrimDb: writable(1.0)`
- `WebUI/src/App.svelte` — add Drive panel with Gain section (2 knobs)

Test: turn gain up/down, no clicks during automation, Mix=0 on later modules passes audio unchanged.

---

### Step 2 — Saturation
**New file:** `Source/DSP/Input/Saturation.h` (header-only)

DSP design:
- tanh waveshaper, normalised: `tanh(x * drive) / tanh(drive)` so ±1 input → ±1 output
- 2× oversampling: linear-interpolation upsample → waveshaper on both sub-samples → average downsample → one-pole IIR AA (k≈0.3)
- Post-saturation tone: 1-pole LP from 500 Hz (tone=0, warm) to ~20 kHz (tone=1, bright), computed per block
- SmoothedValue for drive, mix, tone (10 ms ramps)

Files to change: same pattern as Step 1 (create header, wire processor, add UI knobs).

Test: mix=0 is transparent, drive sweep adds harmonics cleanly, no aliasing artifacts.

---

### Step 3 — TimbreFilter
**New file:** `Source/DSP/Input/TimbreFilter.h` (header-only)

DSP design:
- Standard biquad LP, Direct Form 2 Transposed, double-precision state
- Coefficients (Audio EQ Cookbook): recomputed each block via `setCutoff()` / `setResonance()`
- Pre-drive via SmoothedValue applied before the filter
- Per-channel state arrays (2 channels)

Files to change: same pattern.

Test: resonant sweep 20 Hz → 20 kHz, stable at Q=10, no divergence near Nyquist.

---

### Step 4 — Vibrato
**New file:** `Source/DSP/Modulation/Vibrato.h` (header-only)
**New directory:** `Source/DSP/Modulation/`

DSP design:
- `std::vector<float>` delay buffer per channel, allocated in `prepare()` (real-time safe)
- Sin LFO (phase accumulator) modulates read position: `readOffset = base * (1 + lfoOut * depth)`
- Base delay = kMaxDepthMs samples (~5 ms) so pitch centre is stable
- Linear interpolation for fractional read position
- FadeIn ramp: resets when depth crosses 0 → positive

Files to change: same pattern + `CMakeLists.txt` needs `Source/DSP/Modulation` in sources list.

Test: depth=0 is fully transparent, FadeIn ramp smooth, rate sweep at moderate depth.

---

### Step 5a — UI Layout
No DSP changes. Visual only.

New Drive panel in `App.svelte` (flex 1.2, before Input panel):
```
Drive
  ── Gain ──────── Gain (dB)  |  Output Trim (dB)
  ── Saturation ── Drive  |  Mix  |  Tone
  ── Timbre ─────  Cutoff  |  Resonance  |  Pre-Drive
  ── Modulation ── Rate  |  Depth  |  Fade In
```

Y Amount knobs grouped below each section (small, labeled "Y Amt").

Output panel additions:
- `macroY` large prominent knob above GestureController
- `gestureModeToggle` button labeled "Classic / Y-Track" next to GestureController

Files to change:
- `WebUI/src/state/store.ts` — add all 21 new stores with correct normalized defaults
- `WebUI/src/App.svelte` — Drive panel + Output panel additions

Test: visual layout, panel sizing, all knobs draggable and labelled correctly.

---

### Step 5b — Bridge
Wire all 21 new params to C++↔JS sync.

Files to change:
- `Source/Plugin/PluginEditor.cpp` — add all 21 params to `timerCallback()` and `sendAllParamsToJS()`

Test: drag each new knob in the UI, confirm the parameter value changes in the DSP (audible with modules from Steps 1–4 active).

---

### Step 5c — Y Position Tracking
Extend `GestureController.svelte` to support continuous Y tracking mode.

Logic:
```
// When gestureModeToggle == 1 (Y-Track):
indexTipY = landmarks[0][8].y   // 0=top, 1=bottom of frame
rawY = 1 - indexTipY            // invert: hand up = high value
smoothedY += 0.2 * (rawY - smoothedY)  // one-pole low-pass
macroY = clamp(smoothedY, 0, 1)
send('macroY', macroY)

// When gestureModeToggle == 0 (Classic): existing up/down unchanged
```

Files to change:
- `WebUI/src/components/GestureController.svelte`

Test: move index finger up/down, `macroY` knob follows in real time. Classic mode still works.

---

### Step 5d — Y Modulation (one parameter at a time)

**Architecture in `processBlock`:**
```cpp
// Modulation formula (same for every parameter):
float effectiveNorm = juce::jlimit(0.f, 1.f,
    apvts.get(baseParam) + macroYNorm * apvts.get(yAmtParam));
module.setParam(effectiveNorm);
```

Where `baseParam` is the normalized APVTS base value and `yAmtParam` is the Y Amount (−1 to +1, stored as normalized 0–1 in APVTS, converted: `yAmt = 2 * norm - 1`).

**Wiring order (one at a time, test each):**
1. `dryWetYAmt` → dryWet
2. `gainYAmt` → gainDb
3. `cutoffYAmt` → timbreCutoff
4. `satDriveYAmt` + `satMixYAmt` → satDrive + satMix (together)
5. `vibratoDepthYAmt` → vibratoDepth
6. `decayYAmt` + `dampingYAmt` → decay + damping (together)

For each: set the Y Amount knob to a small positive value (e.g. 0.3), move the hand, tune the feel.

Files to change: `Source/Plugin/PluginProcessor.cpp` only (add effective-value calculation per parameter).

---

## Expected Sonic Character (end state)

| macroY | Character |
|--------|-----------|
| 0 | Baseline knob values, gesture adds nothing |
| mid | Gentle modulation of whichever Y Amounts are non-zero |
| 1 | Full modulation offset applied to all wired parameters |

The feel is expressive and continuous, not a preset switch. Each Y Amount knob gives independent, tunable control over how much the gesture affects that dimension of the sound.

---

## File Map

```
Source/
  Parameters/
    ParameterIDs.h        ✅ Step 0
    ParameterLayout.h     ✅ Step 0
  Plugin/
    PluginProcessor.h     Steps 1–4
    PluginProcessor.cpp   ✅ Step 0 (saveDry slot), Steps 1–4, 5d
    PluginEditor.cpp      Steps 1–4 (timer sync), Step 5b
  DSP/
    Input/
      GainModule.h        Step 1
      Saturation.h        Step 2
      TimbreFilter.h      Step 3
    Modulation/
      Vibrato.h           Step 4
WebUI/src/
  types/parameters.ts     ✅ Step 0
  state/store.ts          Steps 1–4 (per step), Step 5a (all)
  App.svelte              Steps 1–4 (per step), Step 5a (Drive panel)
  components/
    GestureController.svelte  Step 5c
CMakeLists.txt            Steps 1–4 (new source files)
```
