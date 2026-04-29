# PosTalk — Architecture

**Version:** 0.3.0
**Type:** JUCE audio plugin (VST3 / AU / Standalone)
**Effect:** Stereo FDN Reverb with expression control via hand gesture
**Build system:** CMake 3.22+ / C++17
**Key deps:** JUCE 8.0.4, Svelte 4, TypeScript, Vite

---

## 1. Project Structure

```
PosTalk/
├── CMakeLists.txt
├── .gitignore
├── docs/
│   ├── ARCHITECTURE.md
│   ├── EXPRESSION_PLAN.md      — DSP + gesture integration roadmap
│   ├── UI_VERSIONS.md          — UI version history and design specs
│   └── SETUP.md
├── Source/
│   ├── Plugin/
│   │   ├── PluginProcessor.h / .cpp  — audio engine + APVTS, processBlock
│   │   ├── PluginEditor.h / .cpp     — WebView host + 30Hz parameter sync
│   │   └── WebviewBridge.h           — juce:// URL protocol handler
│   ├── Parameters/
│   │   ├── ParameterIDs.h            — 52 string ID constants
│   │   └── ParameterLayout.h         — APVTS layout (all ranges + defaults)
│   └── DSP/
│       ├── Input/
│       │   ├── GainModule.h          — input gain + output trim (Steps 1)
│       │   ├── Saturation.h          — tanh waveshaper, 2× virtual OS (Step 2)
│       │   ├── TimbreFilter.h        — resonant biquad LP, DF2T (Step 3)
│       │   ├── InputFilter.h / .cpp  — hi/lo Butterworth biquads (existing)
│       │   └── Predelay.h / .cpp     — 0–500 ms stereo pre-reverb delay
│       ├── Modulation/
│       │   └── Vibrato.h             — pitch vibrato via delay + sin LFO (Step 4)
│       ├── Decay/
│       │   ├── FDNReverb.h / .cpp    — 8-line FDN core
│       │   └── Freeze.h              — state carrier for freeze mode
│       ├── DiffusionNetwork/
│       │   ├── DiffusionNetwork.h / .cpp — 4-stage Schroeder input diffusion
│       │   ├── DiffusionStage.h          — stereo allpass stage
│       │   ├── CrossoverFilter.h / .cpp  — per-line frequency-dependent damping
│       │   ├── Chorus.h / .cpp           — LFO-modulated stereo delay
│       │   ├── DelayLine.h               — circular buffer + linear interpolation
│       │   ├── FeedbackMatrix.h          — Hadamard 8×8 matrix
│       │   ├── LFO.h                     — phase-accumulator sine oscillator
│       │   └── SmoothedValue.h           — exponential parameter smoother
│       ├── EarlyReflections/
│       │   ├── EarlyReflections.h / .cpp — 8-tap comb-like reflections
│       │   └── SpinModulator.h / .cpp    — early reflection modulation
│       └── Output/
│           ├── DryWetMixer.h             — per-sample crossfade with smoothing
│           └── StereoWidener.h           — mid-side width control
└── WebUI/
    ├── vite.config.ts
    ├── src/
    │   ├── App.svelte                — root layout (proto inspector, v0.2)
    │   ├── bridge/bridge.ts          — juce:// protocol, C++↔JS glue
    │   ├── state/store.ts            — per-parameter Svelte writable stores
    │   ├── types/parameters.ts       — ParameterId union type (52 IDs)
    │   ├── utils/format.ts           — time/frequency display helpers
    │   └── components/
    │       ├── Number.svelte         — scrollable/typeable numeric box
    │       ├── Knob.svelte           — rotary dial (glassmorphic)
    │       ├── Toggle.svelte         — on/off button with LED
    │       ├── Selector.svelte       — segmented option picker
    │       ├── KnobDiscrete.svelte   — labeled dropdown selector
    │       ├── Tooltip.svelte        — hover parameter info
    │       ├── Camera.svelte         — MediaDevices camera feed
    │       ├── FilterGraph.svelte    — interactive biquad frequency response
    │       └── GestureController.svelte — MediaPipe hand landmark tracking
    └── dist/index.html               — built Svelte output (embedded in binary)
```

---

## 2. Audio Signal Chain

Full processing order per audio block:

```
Input Buffer (stereo)
  │
  ├─ [GainModule]       Input gain (−24→+12 dB) + output trim (−12→0 dB)       Step 1
  ├─ [Saturation]       tanh waveshaper, 2× virtual oversampling, tone LP       Step 2
  ├─ [TimbreFilter]     Resonant biquad LP, DF2T, double-precision state         Step 3
  ├─ [Vibrato]          Sin LFO delay modulation, fade-in ramp                   Step 4
  │
  ├─ dryWetMixer.saveDry()   ← dry signal captured here
  │
  ├─ [InputFilter]      Hi-pass (Lo Cut) + Lo-pass (Hi Cut) Butterworth biquads
  ├─ [Predelay]         Stereo delay 0–500 ms
  ├─ [EarlyReflections] 8-tap comb-like delays with spin modulation (additive)
  ├─ [FDNReverb]        8-line feedback delay network (main reverb tail)
  │     ├─ DiffusionNetwork    4-stage Schroeder allpass pre-diffusion
  │     ├─ DelayLine[8]        Mutually prime delays (557–1019 samples @ 44.1kHz)
  │     ├─ CrossoverFilter[8]  Per-line frequency-dependent decay (one-pole LP)
  │     ├─ LFO[8]              Per-line pitch modulation (staggered 0.31–0.56 Hz)
  │     └─ FeedbackMatrix      Normalized Hadamard 8×8 (±1/√8)
  ├─ [Reflect/Diffuse Gains]  Per-component level trim (dB)
  ├─ [Chorus]           LFO-modulated stereo delay (adds movement)
  ├─ [StereoWidener]    Mid-side width control (0–120°)
  └─ [DryWetMixer]      Per-sample smooth crossfade with saved dry signal
```

**Real-time safety:**
- No heap allocations in `processBlock()` — all buffers pre-allocated at `prepareToPlay()`
- `juce::ScopedNoDenormals` active in audio callback
- All parameters use 10 ms `juce::SmoothedValue` ramps to prevent clicks
- Outer-sample / inner-channel loop pattern across all new modules

---

## 3. DSP Modules

### GainModule (Step 1)
Two `SmoothedValue<float>` (10ms): one for `gainDb` → linear, one for `outputTrimDb` → linear. Per-sample multiply: `sample *= smoothGain * smoothTrim`. Header-only.

### Saturation (Step 2)
**Algorithm:** tanh waveshaper normalized to ±1 output: `tanh(x·d) / tanh(d)`. Drive maps 0→1 to d=0.001→10.

**2× virtual oversampling:** linear-interpolation upsample → waveshaper on both sub-samples → average downsample → one-pole anti-aliasing LP (k=0.3).

**Post-tone:** 1-pole LP swept from 500 Hz (tone=0, warm) to ~20 kHz (tone=1, bright), computed once per block. SmoothedValues for drive/mix/tone (10ms). Header-only.

### TimbreFilter (Step 3)
Standard biquad LP — Audio EQ Cookbook coefficients, Direct Form 2 Transposed, **double-precision state** arrays.

Coefficients computed once per block (cutoff and Q smoothed across blocks via `skip()`). Pre-drive gain applied per-sample before the filter. Cutoff clamped to `0.49 × sr` to prevent Nyquist instability. Header-only.

### Vibrato (Step 4)
`std::vector<float>` circular delay buffer per channel (allocated in `prepare()`). Base delay = `kMaxDepthMs = 5 ms` keeps pitch center stable.

**LFO:** phase accumulator → `sin(phase × 2π)`. Read position = `baseDelay + lfo × effectiveDepth × baseDelay`. Linear interpolation for fractional index.

**FadeIn ramp:** detects 0→positive depth crossing, resets a linear ramp over `vibratoFadeIn` ms, scales effective depth during ramp. Header-only.

### FDNReverb — Core Reverb Engine

8-line feedback delay network. Per-sample loop:
1. Diffuse input through DiffusionNetwork
2. Read from 8 delay lines (LFO modulation ±2 samples per line)
3. Apply CrossoverFilter (frequency-dependent decay per line)
4. Apply RT60 gain: `g = 10^(−3 × delayTime / RT60)`
5. Mix through Hadamard matrix (Walsh-Hadamard, O(N log N))
6. Write feedback + input back to lines
7. Output: even lines → L, odd lines → R, scaled 1/√8

**Base delays (mutually prime @ 44.1kHz):** 557, 617, 683, 743, 809, 877, 947, 1019 samples (≈12–23ms)

### DiffusionNetwork
4 cascaded Schroeder allpass stages. Prime delay pairs (L/R):
151/167 → 211/239 → 379/397 → 479/509 samples

### CrossoverFilter
One-pole LP per FDN line: `y[n] = c·y[n-1] + (1−c)·x[n]`. Simulates frequency-dependent absorption.

### InputFilter
Two independent 2nd-order Butterworth biquads (DF2):
- **Hi-pass (Lo Cut):** 50–18000 Hz, Q=0.7071
- **Lo-pass (Hi Cut):** 50–18000 Hz, Q=0.5–9.0

### EarlyReflections
8 stereo delay taps with exponential gain decay and spin modulation.
Gains: `[0.85, 0.70, 0.60, 0.50, 0.40, 0.35, 0.28, 0.22]`
Tap delays (L/R prime): 89/107 → 809/839 samples.

### Chorus
Two LFO-modulated delay lines (L/R). Base delay 12ms, max modulation ±6ms.

### DryWetMixer
Per-sample lerp between dry and wet: `out[i] = lerp(dry[i], wet[i], w_i)` where `w_i` ramps across the block. Prevents zipper noise.

### StereoWidener
Mid-side: `mid = (L+R)/2`, `side = (L−R)/2 × width`. Width derived from `stereo` param (0–120°).

---

## 4. Parameters (52 total)

All stored in APVTS, communicated in normalized 0–1. `getRawParameterValue()` returns actual units (Hz, dB, ms).

### Input Stage — Gain (Step 1)
| ID | Range | Default |
|----|-------|---------|
| `gainDb` | −24–+12 dB | 0 dB |
| `outputTrimDb` | −12–0 dB | 0 dB |

### Input Stage — Saturation (Step 2 / Module 3)
| ID | Range | Default |
|----|-------|---------|
| `satDrive` | 0–1 | 0 |
| `satMix` | 0–100% | 0% |
| `satTone` | 0–1 | 1.0 (bright) |

### Input Stage — Timbre Filter (Step 3 / Module 2)
| ID | Range | Default |
|----|-------|---------|
| `timbreCutoff` | 20–20000 Hz (skew 0.3) | 8000 Hz |
| `timbreResonance` | 0.5–10 (skew 0.5) | 0.7071 |
| `timbreDrivePre` | 0–12 dB | 0 dB |

### Input Stage — Vibrato (Step 4 / Module 5)
| ID | Range | Default |
|----|-------|---------|
| `vibratoRate` | 0.1–8 Hz (skew 0.5) | 2 Hz |
| `vibratoDepth` | 0–1 | 0 |
| `vibratoFadeIn` | 0–500 ms (skew 0.5) | 100 ms |

### Input Filter
| ID | Range | Default |
|----|-------|---------|
| `loCutEnabled` | Bool | false |
| `hiCutEnabled` | Bool | false |
| `loCutFreq` | 50–18000 Hz (skew 0.3) | 80 Hz |
| `hiCutFreq` | 50–18000 Hz (skew 0.3) | 8000 Hz |
| `hiCutQ` | 0.5–9.0 (skew 0.5) | 0.7071 |
| `predelay` | 0–500 ms (skew 0.5) | 20 ms |

### Early Reflections
| ID | Range | Default |
|----|-------|---------|
| `erEnabled` | Bool | false |
| `erAmount` | 2–55 (skew 0.5) | 10 |
| `erRate` | 0.07–1.3 Hz (skew 0.5) | 0.5 Hz |
| `erShape` | 0–1 | 0.5 |
| `reflectGain` | −30–+6 dB | 0 dB |

### FDN / Diffusion
| ID | Range | Default |
|----|-------|---------|
| `reverbMode` | Int 0–1 | 0 |
| `highFilterType` | Bool (LP / High Shelf) | false |
| `crossoverFreq` | 200–8000 Hz (skew 0.5) | 3000 Hz |
| `diffusion` | 0–1 | 0.6 |
| `scale` | 0–1 | 0.5 |
| `damping` | 0–1 | 0.5 |
| `feedback` | 0–1 | 0.75 |
| `decay` | 200–60000 ms (skew 0.3) | 1500 ms |
| `size` | 0–1 (log display 0.22–500) | 0.5 |
| `smooth` | Int 0–3 (Off/Low/Med/High) | 0 |
| `freeze` | Bool | false |
| `flatEnabled` | Bool | false |
| `cutEnabled` | Bool | false |
| `diffuseGain` | −30–+6 dB | 0 dB |

### Chorus
| ID | Range | Default |
|----|-------|---------|
| `chorusEnabled` | Bool | false |
| `chorusAmount` | 0.01–4.0 | 0.2 |
| `chorusRate` | 0.01–8.0 Hz (skew 0.5) | 1.5 Hz |

### Output
| ID | Range | Default |
|----|-------|---------|
| `stereo` | 0–120° | 120° |
| `density` | Int 0–3 | 3 (High) |
| `dryWet` | 0–100% | 50% |

### Gesture / Macro Y (wired in Step 5d)
| ID | Range | Default |
|----|-------|---------|
| `macroY` | 0–1 | 0 |
| `gestureModeToggle` | Bool | false |
| `gainYAmt` | −1–+1 | 0 |
| `cutoffYAmt` | −1–+1 | 0 |
| `satDriveYAmt` | −1–+1 | 0 |
| `satMixYAmt` | −1–+1 | 0 |
| `vibratoDepthYAmt` | −1–+1 | 0 |
| `dryWetYAmt` | −1–+1 | 0 |
| `decayYAmt` | −1–+1 | 0 |
| `dampingYAmt` | −1–+1 | 0 |

---

## 5. C++ ↔ JavaScript Bridge

### JS → C++ (user interaction)
```typescript
// bridge.ts
window.location.href = `juce://setparameter?name=${id}&value=${normalizedValue}`;
```
```cpp
// WebviewBridge.h — pageAboutToLoad()
// Parses URL, calls: apvts.getParameter(id)->setValueNotifyingHost(value)
```

### C++ → JS (30Hz polling timer)
`PluginEditor` fires at 30Hz. If APVTS value changed > 0.001 since last sync, calls `evaluateJavascript()`:
```cpp
evaluateJavascript("if (window.setParameterValue) { window.setParameterValue('" + id + "', " + value + "); }");
```
```typescript
// bridge.ts — registered at startup
(window as any).setParameterValue = (id: ParameterId, value: number) => {
  (params as any)[id]?.set(value);  // updates Svelte store
};
```

**Note:** `juce://` only works inside the plugin WebView, not the Vite dev server. Bridge includes a `http://` context guard.

---

## 6. Frontend (Svelte WebUI)

### State Management

52 independent `writable` Svelte stores (normalized 0–1). Derived stores compute display units:

```typescript
export const params = { decay: writable(0.42), satDrive: writable(0), ... };

// normalized → display
export const decayMs = derived(params.decay, $v => Math.round(200 + 59800 * Math.pow($v, 1/0.3)));
export const crossoverHz = derived(params.crossoverFreq, $v => Math.round(200 + 7800 * Math.pow($v, 1/0.5)));
```

Display → normalized conversion uses `nl(v, min, max)` / `ns(v, min, max, skew)` helpers defined inline in App.svelte.

### Current UI — Proto Inspector (v0.2)

```
┌─ Header: POSTALK — REVERB·01 ──────────────────── [BYPASS] ─┐
│                                                               │
│  ┌─ Presets ─┐  ┌──────── Camera ────────┐  ┌─ Gestures ──┐ │
│  │ 1  2  3   │  │                        │  │ 🤙  ✊       │ │
│  │ 4  5  6   │  │                        │  │ Gain (dB)   │ │
│  └───────────┘  └────────────────────────┘  │ Dry/Wet (%) │ │
│                                              └─────────────┘ │
│  ┌─ Reverb ──────────────┐ ┌─ Sat ─┐ ┌─ Timbre ─┐ ┌─ Vib ─┐│
│  │ Core / Mix / FDN /    │ │ Gain  │ │ Cutoff   │ │ Rate  ││
│  │ Gates / ER / Chorus / │ │ Drive │ │ Resonance│ │ Depth ││
│  │ Input Filter          │ │ ...   │ │ Pre-Drive│ │ Fade  ││
│  └───────────────────────┘ └───────┘ └──────────┘ └───────┘│
└───────────────────────────────────────────────────────────────┘
```

All parameters displayed as `Number.svelte` boxes — scroll wheel or double-click to edit, real units shown (Hz, dB, ms, °, %). Each panel scrolls independently (`overflow-y: auto`, `max-height: calc(100vh - 420px)`).

### Components

| Component | Role |
|-----------|------|
| `Number.svelte` | Scroll/type numeric input, tooltip, context menu |
| `Knob.svelte` | Rotary dial with orange arc + glow (glassmorphic v0.1) |
| `Toggle.svelte` | On/off with orange LED |
| `Selector.svelte` | Segmented tab picker |
| `KnobDiscrete.svelte` | Labeled dropdown |
| `Camera.svelte` | `getUserMedia` video feed |
| `FilterGraph.svelte` | Interactive biquad frequency canvas |
| `GestureController.svelte` | MediaPipe hand landmark tracking |
| `Tooltip.svelte` | Hover info overlay |

---

## 7. Build & Deployment

```bash
# Full build
cd build && cmake .. && make -j$(sysctl -n hw.ncpu)

# Frontend only
cd WebUI && npm run build
```

**Web asset embedding:** Svelte → `WebUI/dist/index.html` → `juce_add_binary_data` → embedded in binary → unpacked to temp dir at runtime → loaded in `WebBrowserComponent`.

**Plugin outputs:**
- Standalone: `build/AudioPlugin02_artefacts/Standalone/`
- VST3: auto-copied to `~/Library/Audio/Plug-Ins/VST3/` post-build
- AU: auto-copied to `~/Library/Audio/Plug-Ins/Components/`

**Window:** 1260×700px default, resizable 450×280 → 1800×1120.

---

## 8. Key Architectural Decisions

| Decision | Rationale |
|----------|-----------|
| 8-line FDN over stereo pair | Richer decorrelation and spatial diffusion |
| Hadamard feedback matrix | Energy-preserving, efficient O(N log N) |
| Per-line LFO modulation | Prevents metallic ringing and mode flutter |
| 4-stage Schroeder pre-diffusion | Spreads transients before FDN, reduces coloration |
| Frequency-dependent per-line decay | Realistic room behavior (highs decay faster) |
| Header-only new DSP modules | No .cpp needed; simpler CMake, faster iteration |
| Outer-sample / inner-channel loop | Both channels share same SmoothedValue step per sample |
| 2× virtual oversampling in Saturation | Reduces aliasing without true oversampling overhead |
| DF2T double-precision state in TimbreFilter | Prevents numerical noise at high Q near Nyquist |
| Vibrato base delay = 5ms | Keeps pitch center stable while LFO swings ±depth |
| Per-sample dry/wet interpolation | Smooth automation without zipper noise |
| 30Hz C++→JS polling | Covers DAW automation without callback complexity |
| `juce://` URL scheme for JS→C++ | Decouples frontend from backend; no native binding needed |
| Per-parameter Svelte stores | Fine-grained reactivity, avoids full-tree re-renders |
| Proto inspector UI (v0.2) | Exposes all 52 params as number boxes for rapid DSP iteration |
| Git branches per UI version | Allows non-destructive switching between design iterations |
