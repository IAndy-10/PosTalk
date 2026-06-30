# PosTalk

A stereo algorithmic reverb plugin with real-time hand gesture control. Gesture tracking via webcam and MediaPipe drives DAW parameters live — no MIDI controllers required.

Built with **C++17 / JUCE 8** for DSP and **Svelte / TypeScript** for the embedded UI.

---

## Features

- **8-line Feedback Delay Network** reverb with Hadamard feedback matrix and prime-spaced delay lines
- **Early reflections** — 8-tap parallel delay with spin modulation and configurable decay shape
- **4-stage Schroeder allpass diffusion** network with variable density
- **Frequency-dependent decay** via per-line crossover filters (LP or high-shelf mode)
- **Chorus and stereo widening** post-processing
- **Gain staging** — input gain and output trim with ±dB control
- **Saturation** — drive, mix, and tone shaping
- **Timbre filter** — XY-controlled cutoff/resonance with pre-drive
- **Vibrato** — rate, depth, and fade-in modulation
- **Pitch Shifter** — continuous or octave-snap frequency shift (110–1760 Hz)
- **Hand gesture control** — webcam + MediaPipe HandLandmarker maps gestures to plugin parameters in real time
- **Neumorphic UI** embedded directly in the plugin window via JUCE WebBrowserComponent
- Self-contained binary — no external web server, no internet required at runtime

---

## Signal Flow

```
INPUT
  │
  ├─► [Gain]               Input gain (gainDb)
  │
  ├─► [InputFilter]        High-pass & low-pass biquad
  │
  ├─► [Timbre]             Resonant filter with pre-drive
  │
  ├─► [Saturation]         Drive + tone shaping, parallel mix
  │
  ├─► [Predelay]           Up to 500 ms stereo delay
  │         │
  │         ├──────────────────────────────────────┐
  │         ▼                                      ▼
  │  [EarlyReflections]                 [DiffusionNetwork]
  │  8-tap parallel delay               4-stage allpass chain
  │  + spin modulation                             │
  │         │                                      ▼
  │         │                             [FDNReverb]
  │         │                             8-line FDN
  │         │                             Hadamard feedback matrix
  │         │                             CrossoverFilter per line
  │         │                             Per-line LFO modulation
  │         │                                      │
  │         └──────────────────────────────────────┤
  │                                                ▼
  │                                      [Chorus]  (optional)
  │                                      [StereoWidener]
  │                                      [Vibrato]
  │                                      [PitchShifter]
  │                                                │
  └──────────────────────────────────► [DryWetMixer]
                                                   │
                                            [OutputTrim]
                                                   │
                                               OUTPUT
```

---

## Parameters

The plugin exposes **56 parameters** in total, organized by functional module.

### Input Filter
| Parameter | Range | Description |
|-----------|-------|-------------|
| Lo Cut Freq | 50–18 000 Hz | High-pass cutoff |
| Lo Cut Enable | On/Off | Engages the high-pass filter |
| Hi Cut Freq | 50–18 000 Hz | Low-pass cutoff |
| Hi Cut Q | 0.5–9.0 | Low-pass resonance |
| Hi Cut Enable | On/Off | Engages the low-pass filter |
| Predelay | 0–500 ms | Delays reverb onset |

### Early Reflections
| Parameter | Range | Description |
|-----------|-------|-------------|
| ER Enable | On/Off | Activates early reflections path |
| ER Amount | 2–55 | Spin modulation depth |
| ER Rate | 0.07–1.3 Hz | Spin modulation rate |
| Shape | 0–1 | Gradual (smooth) to rapid (dense early, quiet tail) |
| Reflect Gain | −30 to +6 dB | Early reflections output level |

### Diffusion / FDN
| Parameter | Range | Description |
|-----------|-------|-------------|
| Reverb Mode | High / Low | Crossover frequency scaling (×1.5 brighter / ×0.6 darker) |
| Crossover Freq | 200–8 000 Hz | Per-line decay filter corner frequency |
| Diffusion | 0–1 | Allpass coefficient; controls echo density |
| Scale | 0–1 | Raw vs. diffused signal blend entering the FDN |
| Damping | 0–1 | High-frequency absorption inside feedback loop |
| Feedback | 0–1 | Global feedback scaling |
| highFilterType | LP / High Shelf | Crossover mode |
| Chorus Enable | On/Off | Engages chorus stage |
| Chorus Amount | 0.01–4.0 | Chorus modulation depth |
| Chorus Rate | 0.01–8.0 Hz | Chorus LFO frequency |
| Diffuse Gain | −30 to +6 dB | FDN reverb output level |

### Decay / Utility
| Parameter | Range | Description |
|-----------|-------|-------------|
| Decay (RT60) | 0.2–60 s | Target reverberation time |
| Smooth | Off / Low / Med / High | Parameter ramp time (1 / 20 / 80 / 200 ms) |
| Size | 0.22–500 | Scales all FDN delay lengths |
| Freeze | On/Off | Sustains reverb tail indefinitely |
| Flat | On/Off | Bypasses crossover filters |
| Cut | On/Off | Blocks new input; sustains current FDN state |

### Output / Width
| Parameter | Range | Description |
|-----------|-------|-------------|
| Stereo Width | 0–120° | Mid-Side width (0° = mono) |
| Density | Sparse / Low / Mid / High | Active allpass diffusion stages (0–4) |
| Dry/Wet | 0–100 % | Dry/processed blend |

### Gain Staging
| Parameter | Range | Description |
|-----------|-------|-------------|
| Input Gain | −24 to +12 dB | Pre-chain input level |
| Output Trim | −12 to 0 dB | Post-chain output level |

### Saturation
| Parameter | Range | Description |
|-----------|-------|-------------|
| Sat Drive | 0–100 % | Drive amount |
| Sat Mix | 0–100 % | Parallel dry/wet for saturation |
| Sat Tone | 0–1 | Tonal character (dark to bright) |

### Timbre
| Parameter | Range | Description |
|-----------|-------|-------------|
| Timbre Cutoff | 20–20 000 Hz | Filter cutoff frequency |
| Timbre Resonance | 0.5–10 | Filter resonance / Q |
| Pre-Drive | 0–12 dB | Drive applied before the timbre filter |

### Vibrato
| Parameter | Range | Description |
|-----------|-------|-------------|
| Vibrato Rate | 0.1–8.0 Hz | LFO frequency |
| Vibrato Depth | 0–1 | Modulation depth |
| Vibrato Fade In | 0–500 ms | Ramp time before vibrato reaches full depth |

### Pitch Shifter
| Parameter | Range | Description |
|-----------|-------|-------------|
| Pitch Frequency | 110–1760 Hz | Shift target frequency (440 Hz = unity) |
| Oct Step | Continuous / Snap | Continuous pitch or octave-locked stepping |

### Performance / Gesture
| Parameter | Range | Description |
|-----------|-------|-------------|
| Sustain Enable | On/Off | Holds reverb tail indefinitely |
| Cut Now | Momentary | Clears FDN state on rising edge |
| Gesture Mode | Classic / Y-Track | Active gesture mapping mode |
| Macro Y | 0–1 | Continuous Y-axis macro from hand tracking |

### Y-Amount Modulation Depths
These scale how much `macroY` affects each target parameter (0–1 per parameter):

`gainYAmt`, `cutoffYAmt`, `satDriveYAmt`, `satMixYAmt`, `vibratoDepthYAmt`, `dryWetYAmt`, `decayYAmt`, `dampingYAmt`

---

## Gesture Control

PosTalk uses **MediaPipe HandLandmarker** running inside the embedded WebView to track hand landmarks in real time from the built-in webcam.

Three gesture modes (priority order):

| Priority | Hands | Gesture | Action |
|----------|-------|---------|--------|
| 1 | Both | Both pointing up | Pitch frequency increases continuously |
| 1 | Both | Both pointing down | Pitch frequency decreases continuously |
| 2 | One | Index up, others curled | Gates ON — engages Sustain |
| 2 | One | Index down, others curled | Gates OFF — releases Sustain |
| 3 | None | — | Idle; gate state persists |

Gate state is **latching** — losing the hand or returning to a neutral pose does not change the current Sustain state. Only the opposite gesture releases it.

All gesture values are normalized 0–1 floats sent to JUCE via the `juce://setparameter` bridge URL scheme. No MIDI, no OSC — everything runs inside the plugin process.

### Hardware Required
| Device | Role |
|--------|------|
| Computer | Runs DAW + PosTalk |
| Built-in Webcam | Captures hand gestures |
| Audio Interface | Audio output |

No internet connection is required at runtime.

---

## Architecture

### DSP (C++)
- `Source/DSP/DiffusionNetwork/` — allpass chain, LFO, delay lines, feedback matrix
- `Source/DSP/EarlyReflections/` — 8-tap delay + spin modulator
- `Source/DSP/Input/` — biquad input filters, gain module
- `Source/DSP/Decay/` — freeze module
- `Source/Plugin/` — JUCE AudioProcessor, WebviewBridge, PluginEditor, CameraHelper

### UI (Svelte + TypeScript)
- `WebUI/src/App.svelte` — root layout (top row: Presets / Camera / Gestures; bottom row: Reverb / Saturation / Timbre / Shimmer)
- `WebUI/src/components/` — Knob, VSlider, LedToggle, NumberPill, XYPad, GestureMap, GestureController, Camera, Lamp, Tooltip, and legacy Neu* components
- `WebUI/src/bridge/bridge.ts` — bidirectional JS ↔ C++ parameter communication
- `WebUI/src/state/store.ts` — per-parameter Svelte writable stores and derived display values
- `WebUI/src/stores/` — camera stream, hand landmarks, tooltip state
- `WebUI/src/types/parameters.ts` — `ParameterId` union type (56 IDs, mirrors `ParameterIDs.h`)

### Bridge
JS → C++ parameter changes navigate to a custom URL scheme intercepted by `WebBrowserComponent`:

```
juce://setparameter?name=decay&value=0.42
```

C++ → JS updates are pushed via `WebBrowserComponent::evaluateJavascript()` on the message thread, keeping the audio thread lock-free.

---

## Technology Stack

| Category | Technology |
|----------|------------|
| DSP | C++17, JUCE 8.0.4 |
| UI | Svelte 5, TypeScript, Tailwind CSS |
| Gesture tracking | MediaPipe Tasks Vision |
| UI build | Vite 7 |
| Build system | CMake 3.22+ |
| Plugin formats | VST3, AU, Standalone |
| Target OS | macOS (primary), Windows (VST3) |
| WebView engine | WKWebView (macOS), CEF (Windows) |

---

## Building

### Prerequisites
- CMake 3.22+
- JUCE 8.0.4
- Node.js (for WebUI)
- Xcode (macOS) or MSVC (Windows)

### Build

```bash
# Build UI and plugin
cmake --build build --target AudioPlugin02_All
```

The Svelte bundle is compiled, embedded into the binary via `juce_add_binary_data`, and included in the final `.vst3` / `.component` bundle — no external files needed at runtime.

### UI development

```bash
cd WebUI
npm install
npm run dev    # Vite dev server (UI preview only, no DSP)
npm run build  # Production bundle
```

---

## Project Status

- Core FDN reverb engine — complete
- Early reflections + diffusion network — complete
- Gain staging (input + output trim) — complete
- Saturation module — complete
- Timbre filter (XY cutoff/resonance + pre-drive) — complete
- Vibrato module — complete
- Pitch Shifter — complete
- Gesture control (index-up/down gates, two-hand pitch) — complete
- Presets (7 factory presets) — complete
- Neumorphic UI — complete
- Y-Track macro gesture mode — in progress (parameters wired, gesture mapping in development)
