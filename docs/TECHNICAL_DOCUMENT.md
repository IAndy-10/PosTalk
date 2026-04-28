# AudioPlugin02 — Technical Document
### FDN Reverb Effect Plugin

---

## 1. Overview & Signal Flow

### What It Does

AudioPlugin02 is a stereo algorithmic reverb plugin built around an 8-line **Feedback Delay Network (FDN)**. It produces dense, smooth, natural-sounding reverb tails with frequency-dependent decay, early reflections, and optional modulation effects. The target use case is studio-quality spatial processing inside a Digital Audio Workstation (DAW).

### Signal Flow

```
INPUT
  │
  ├─► [InputFilter]  ─ High-pass & low-pass biquad filters
  │
  ├─► [Predelay]  ─ Up to 500 ms stereo delay
  │           │
  │           ├──────────────────────────────────────────┐
  │           │                                          │
  │           ▼                                          ▼
  │   [EarlyReflections]                    [DiffusionNetwork]
  │   8-tap parallel delay                  4-stage allpass chain
  │   + spin modulation                     (Schroeder)
  │           │                                          │
  │           │ erOutputBuffer                           │ diffused signal
  │           │                                          ▼
  │           │                              [FDNReverb]
  │           │                              8-line FDN
  │           │                              Hadamard feedback matrix
  │           │                              CrossoverFilter per line
  │           │                              Per-line LFO modulation
  │           │                                          │
  │           │         diffuseGain (dB)                 │
  │           └──────────────────────────────────────────┤
  │                     reflectGain (dB)                 │
  │                                                      ▼
  │                                           [Chorus]  (optional)
  │                                           LFO-modulated delay
  │                                                      │
  │                                           [StereoWidener]
  │                                           Mid-Side width control
  │                                                      │
  └───────────────────────────────────────► [DryWetMixer]
                                             Per-sample interpolation
                                                      │
                                                   OUTPUT
```

### User-Tunable Parameters

**Panel 1 — Input**

| Parameter | Range | Effect |
|-----------|-------|--------|
| Lo Cut Freq | 50–18 000 Hz | High-pass cutoff; removes low-frequency content entering the reverb |
| Lo Cut Enable | On/Off | Engages the high-pass filter |
| Hi Cut Freq | 50–18 000 Hz | Low-pass cutoff; darkens input before reverb |
| Hi Cut Q | 0.5–9.0 | Resonance of the low-pass filter |
| Hi Cut Enable | On/Off | Engages the low-pass filter |
| Predelay | 0–500 ms | Delays reverb onset; increases perceived room size |

**Panel 2 — Early Reflections**

| Parameter | Range | Effect |
|-----------|-------|--------|
| ER Enable | On/Off | Activates the early reflections path |
| ER Amount | 2–55 | Spin modulation depth; adds movement to early reflections |
| ER Rate | 0.07–1.3 Hz | Spin modulation rate |
| Shape | 0–1 | Blends between gradual (smooth decay) and rapid (dense early, quiet tail) tap gain profiles |
| Reflect Gain | −30 to +6 dB | Independent level of the early reflections output |

**Panel 3 — Diffusion / FDN Controls**

| Parameter | Range | Effect |
|-----------|-------|--------|
| Reverb Mode | High / Low | Sets crossover frequency scaling (×1.5 brighter or ×0.6 darker) |
| Crossover Freq | 200–8 000 Hz | Corner frequency of per-line decay filter |
| Diffusion | 0–1 | Allpass coefficient in diffusion stages; increases echo density |
| Scale | 0–1 | Blend of raw vs. diffused signal entering the FDN |
| Damping | 0–1 | High-frequency absorption inside the FDN feedback loop |
| Feedback | 0–1 | Global feedback scaling; controls reverb energy retention |
| highFilterType | false = LP / true = High Shelf | Crossover mode: one-pole LP vs. first-order −6 dB high shelf |
| Chorus Enable | On/Off | Engages the modulated chorus stage |
| Chorus Amount | 0.01–4.0 | Depth of chorus modulation |
| Chorus Rate | 0.01–8.0 Hz | LFO frequency of the chorus |
| Diffuse Gain | −30 to +6 dB | Level of the FDN reverb output |

**Panel 4 — Decay**

| Parameter | Range | Effect |
|-----------|-------|--------|
| Decay (RT60) | 0.2–60 s | Target reverberation time (time to −60 dB) |
| Smooth | Off / Low / Med / High | Ramp time for parameter changes (1 / 20 / 80 / 200 ms) |
| Size | 0.22–500 (log) | Scales all FDN delay lengths; controls perceived room size |
| Freeze | On/Off | Sustains the reverb tail indefinitely (g → 0.9999) |
| Flat | On/Off | Bypasses CrossoverFilters; preserves existing spectral content |
| Cut | On/Off | Blocks new input; FDN sustains its current state |

**Panel 5 — Output**

| Parameter | Range | Effect |
|-----------|-------|--------|
| Stereo Width | 0–120° | Mid-Side width control; 0° = mono, 120° = maximum stereo spread |
| Density | Sparse / Low / Mid / High | Number of active allpass diffusion stages (0–4) |
| Dry/Wet | 0–100 % | Blend between unprocessed and fully processed signal |

---

## 2. DSP Layer

### 2.1 Feedback Delay Network — FDNReverb

**Purpose:** Core reverb engine. Implements an 8-line FDN with Hadamard feedback matrix, per-line decay filters, and LFO modulation.

#### Mathematical Foundation

An N-line FDN is defined as:

```
s[n] = A · s[n - d] + b · x[n]
y[n] = c^T · s[n]
```

where `s[n]` is the N-element delay output vector, `d = [d₀ … d₇]` are the delay lengths, `A` is the N×N feedback matrix, `b` is the input mixing vector, and `c` is the output mixing vector.

**Decay per line** follows the RT60 formula:

```
g = 10^( -3 · (dᵢ / Fs) / RT60 )
```

where `Fs` is the sample rate and RT60 is the target decay time in seconds. The final per-line feedback gain is `gᵢ = g · feedback_param`, clamped to ≤ 0.9999.

**Delay lengths** (base values at 44.1 kHz):

```
d = [557, 617, 683, 743, 809, 877, 947, 1019]  (samples)
```

All values are mutually coprime primes or near-primes, preventing resonant comb modes. The `size` parameter scales every delay: `dᵢ_eff = dᵢ × (0.5 + size)`.

**Feedback matrix** — normalized 8×8 Hadamard via fast Walsh-Hadamard transform:

```
H₈ = (1/√8) · [ H₄   H₄ ]
               [ H₄  -H₄ ]
```

The normalization factor `1/√8` makes the matrix unitary (energy-preserving), preventing the FDN from amplifying or attenuating over time from the matrix alone.

#### Frequency-Dependent Decay (CrossoverFilter)

Each FDN line passes its feedback through a per-line `CrossoverFilter`, implementing two modes:

**LP Mode (default):**
```
y[n] = coeff · x[n] + (1 - coeff) · y[n-1]

coeff = 1 - damping · (1 - baseCoeff)
baseCoeff = 1 - exp(-2π · f_c / Fs)
```
Higher `damping` → lower `coeff` → stronger HF attenuation inside the loop.

**High-Shelf Mode (−6 dB):**
```
y_lp[n] = 0.5 · x[n] + 0.5 · z1_lp     (one-pole LP, feedforward)
y[n]    = 0.5 · x[n] + 0.5 · y_lp[n]   (shelf = mix of LP + dry)
```
This delivers a gentle −6 dB/octave roll-off above the crossover frequency.

**Reverb Mode scaling** modifies the effective cutoff:
- Mode 0 (High): `f_eff = f_crossover × 1.5` → brighter reverb
- Mode 1 (Low): `f_eff = f_crossover × 0.6` → darker reverb

#### Per-Line Modulation

Each line has a sine LFO with a base frequency staggered across lines to decorrelate the delay-length variations:

```
f_i = 0.31 + i × 0.08  Hz,   depth = 2 samples
delay_i_mod = delay_i + 2 · sin(2π · f_i · n / Fs)
```

This 2-sample vibrato prevents metallic flutter tones from periodic delay structures.

#### Key Functions

| Function | Description |
|----------|-------------|
| `prepare(sr, block)` | Allocates all 8 delay buffers, initializes filters and LFOs |
| `process(buffer, diffusedBuffer, params)` | Runs one block through the full FDN loop |
| `applyFeedbackMatrix(v[])` | In-place fast Walsh-Hadamard transform + normalize |
| `updateDecayGains(rt60, sr)` | Recalculates per-line `g` from RT60 and delay lengths |

#### Design Choices

- **Prime delays:** Avoids echo coloration and comb-filter resonances.
- **Hadamard matrix:** Provides maximum mixing per recursion step in O(N log N), preferred over random matrices for predictable energy behavior.
- **Per-line LP filters:** Mimics air absorption (HF decays faster than LF), which is the primary cause of natural reverb's tonal evolution.

---

### 2.2 Filters

#### InputFilter

**Purpose:** Pre-conditions the dry signal before entering the reverb with a high-pass and low-pass filter.

**Implementation:** Second-order biquad IIR in Direct Form II:

```
w[n]  =  x[n] - a₁·w[n-1] - a₂·w[n-2]
y[n]  =  b₀·w[n] + b₁·w[n-1] + b₂·w[n-2]
```

**High-pass (Lo Cut):** Butterworth Q = 1/√2 ≈ 0.7071

```
α   = sin(ω₀) / (2 · Q),   ω₀ = 2π · f_c / Fs
b₀  =  (1 + cos(ω₀)) / 2
b₁  = -(1 + cos(ω₀))
b₂  =  (1 + cos(ω₀)) / 2
a₁  = -2·cos(ω₀) / (1 + α)
a₂  = (1 - α) / (1 + α)
```

**Low-pass (Hi Cut):** Same structure with variable Q (0.5–9.0) and inverted `b` coefficients.

Denormal flushing is applied to state variables to prevent CPU spikes from subnormal IEEE 754 values.

#### CrossoverFilter (per FDN line)

Described in §2.1 above. The CrossoverFilter is also used with `flatEnabled` mode during Freeze: when flat is active, filters are bypassed to sustain the existing spectral content without further HF roll-off.

---

### 2.3 Diffusion Network

**Purpose:** Increases echo density before the FDN by smearing discrete reflections into a more continuous sound. Without diffusion, early FDN outputs can sound grainy or "fluttery."

**Algorithm:** Chain of up to 4 first-order Schroeder allpass filters in series. Each stage:

```
y[n] = -g · x[n] + x[n - D] + g · y[n - D]
```

where `g` is the allpass coefficient (0–1) and `D` is the delay length (in samples). The allpass transfer function has unity magnitude across all frequencies — it only disperses phase — which increases apparent density without affecting tonal balance.

**Delay lengths (stereo prime pairs):**

| Stage | Left (samples) | Right (samples) |
|-------|----------------|-----------------|
| 0 | 151 | 167 |
| 1 | 211 | 239 |
| 2 | 379 | 397 |
| 3 | 479 | 509 |

Asymmetric L/R delays decorrelate the two channels for natural stereo diffusion.

**Density selector** controls how many stages are active:

| Density | Active stages | Character |
|---------|---------------|-----------|
| Sparse | 0 | No diffusion, discrete echoes |
| Low | 1 | Light smearing |
| Mid | 2 | Moderate density |
| High | 4 | Maximum smoothness |

The `scale` parameter blends diffused and raw signals entering the FDN:

```
input_FDN = (1 - scale) · raw + scale · diffused
```

---

### 2.4 Early Reflections

**Purpose:** Simulates the initial discrete reflections that arrive before the diffuse reverb tail. These shape the perceived room geometry and impulse character.

**Architecture:** 8 parallel delay taps per channel, summed:

```
y[n] = Σᵢ gᵢ · x[n - dᵢ]    (i = 0..7)
```

**Delay taps (samples at 44.1 kHz):**
- Left:  [89, 151, 211, 347, 431, 557, 683, 809]
- Right: [107, 167, 241, 367, 461, 587, 709, 839]

**Tap gain profiles (Shape parameter):**
- Gradual (shape=0): `[0.85, 0.70, 0.60, 0.50, 0.40, 0.35, 0.28, 0.22]`
- Rapid (shape=1): `[0.90, 0.55, 0.33, 0.20, 0.12, 0.07, 0.04, 0.02]`
- Intermediate values blend linearly between the two profiles.

**Spin modulation** applies quadrature LFOs (90° phase offset) to vary the effective tap delays:

```
modL[n] = amount · sin(2π · rate · n / Fs)
modR[n] = amount · cos(2π · rate · n / Fs)
```

A 15% cross-mix is added: `outL += 0.15 · modR`, `outR += 0.15 · modL`, creating slight stereo movement without full chorus artifacts.

---

### 2.5 Auxiliary DSP Modules

**DelayLine** — Circular buffer with integer read (`read(D)`) and linear-interpolated read (`readInterpolated(D_float)`) for sub-sample accuracy in the chorus.

**Chorus** — Stereo LFO-modulated delay:
```
delay_L[n] = 12 ms + depth · sin(2π · rate · n / Fs)
delay_R[n] = 12 ms + depth · cos(2π · rate · n / Fs)
out[n] = in[n] + amount · (wet[n] - in[n])
```
Maximum depth is 6 ms. The quadrature LFO pair widens the stereo image during modulation.

**StereoWidener** — Mid-Side processing:
```
M = (L + R) · 0.5
S = (L - R) · 0.5 · width
L_out = M + S,   R_out = M - S
```

**DryWetMixer** — Per-sample linear interpolation across the block to prevent zipper noise on automation.

---

## 3. WebUI Layer

### Framework: Svelte

The UI is built with **Svelte** compiled via **Vite** and rendered inside the plugin window using JUCE's `WebBrowserComponent`.

**Why Svelte:**
- **No virtual DOM:** Svelte compiles to vanilla JavaScript, eliminating runtime overhead — important for a UI running inside an audio process.
- **Reactive stores natively:** Svelte's `writable` and `derived` stores map directly to the one-store-per-parameter model used for APVTS parameter binding.
- **Small bundle size:** Compiled output has no framework runtime dependency, reducing initial load time inside the WebView.

**Limitations:**
- JUCE's embedded WebView (CEF/WKWebView) may have CSS/JS feature gaps compared to a full browser, requiring some defensive styling.
- Hot-reload development requires a separate Vite dev server, which does not interact with the running plugin.

### Component Architecture

```
App.svelte
├── TopBar           — Plugin title, freeze/flat/cut buttons
├── Panel1           — Input (filter graph, knobs)
├── Panel2           — Early Reflections (knobs, ER enable)
├── Panel3           — Diffusion (SVG graph, selectors, knobs)
├── Panel4           — Decay (RT60 knob, freeze controls)
└── Panel5           — Output (width, dry/wet)

Shared components:
├── NeuKnob          — Rotary control with SVG arc
├── NeuButton        — Toggle with LED dot indicator
├── NeuSelector      — Discrete option picker
├── NeuNumber        — Numeric text input with spinner
├── NeuKnobDiscrete  — Integer selector as rotary
├── FilterGraph      — SVG frequency response visualizer
├── NeuDiffusionNetworkGraph — Parameter-linked SVG node graph
└── Tooltip          — Global tooltip overlay
```

### State → DSP Mapping

Each parameter has a dedicated `writable<number>` store in `store.ts` holding the **normalized value** (0–1). A corresponding `derived` store computes the **display value** using the inverse of JUCE's skew formula:

```typescript
// JUCE skew formula: normalized = ((display - min) / range)^skew
// Inverse (display from normalized):
export const loCutHz = derived(loCutFreq,
  v => 50 + 17950 * Math.pow(v, 1 / 0.3)
);

export const decayDisplay = derived(decayParam,
  v => 200 + 59800 * Math.pow(v, 1 / 0.3)   // ms
);

export const sizeDisplay = derived(sizeParam,
  v => 0.22 * Math.pow(500 / 0.22, v)        // log mapping
);
```

When a knob changes, it calls `bridge.sendParameterChange(id, normalizedValue)` — the store holds the normalized value, while the derived store drives what the user sees.

### Visual Design

The UI uses a **neumorphic** aesthetic: warm beige background (`#ede6da`), inset/outset shadows for depth, and dark canvases for interactive graphs. Knobs render SVG arcs for the value indicator. Active buttons emit a CSS `box-shadow` glow. The diffusion network panel renders an interactive SVG graph whose node positions shift with the `diffusion`, `damping`, and `feedback` parameters, giving a visual representation of signal energy distribution.

---

## 4. Bridge Logic: DSP ↔ WebUI

### Architecture

Communication between the Svelte UI (JavaScript) and the C++ audio engine uses JUCE's `WebBrowserComponent` URL-intercept mechanism — no WebSocket or shared memory is involved.

```
   Svelte (JS)                      C++ Plugin
  ─────────────                  ──────────────────
  bridge.sendParameterChange()
    → navigates to custom URL:
      juce://setparameter?         WebBrowserComponent
         name=decay&value=0.42  →  urlAboutToBeLoaded()
                                     intercepts URL
                                     parses name + value
                                     calls APVTS.getParameter()
                                            .setValueNotifyingHost()
```

**JS → C++ (parameter change):**

```typescript
// bridge.ts
function sendParameterChange(id: string, value: number) {
  window.location.href =
    `juce://setparameter?name=${id}&value=${value}`;
}
```

**C++ intercepts in WebviewBridge:**

```cpp
bool urlAboutToBeLoaded(const juce::String& url) override {
    if (url.startsWith("juce://setparameter")) {
        auto name  = url.fromFirstOccurrenceOf("name=",  false, false)
                       .upToFirstOccurrenceOf("&", true, false);
        auto value = url.fromLastOccurrenceOf("value=", false, false)
                       .getFloatValue();
        if (auto* p = processor.apvts.getParameter(name))
            p->setValueNotifyingHost(value);
        return false;  // block navigation
    }
    return true;
}
```

**C++ → JS (host-driven parameter updates):**
The `WebBrowserComponent::evaluateJavascript()` method is called from the plugin processor to push state changes back to the UI (e.g., when a DAW automation lane updates a parameter).

### Threading & Real-Time Safety

- **JS → C++** URL intercept fires on the **message thread** — safe for APVTS `setValueNotifyingHost()`.
- **Audio thread** reads parameter values via `apvts.getRawParameterValue()` atomic pointers and `SmoothedValue` ramps — no locks, no allocations.
- **C++ → JS** calls are dispatched on the message thread via `juce::MessageManager::callAsync()`, never from the audio thread.
- The `Smooth` selector controls `LinearSmoothedValue` ramp times, preventing audio clicks from large parameter jumps.

---

## 5. Technology Overview

| Category | Technology |
|----------|------------|
| DSP language | C++17 |
| UI language | TypeScript, Svelte |
| Audio framework | JUCE 8.0.4 |
| UI build tool | Vite 5 |
| Build system | CMake 3.22+ |
| Plugin formats | VST3, AU, Standalone |
| Target OS | macOS (primary), Windows (VST3) |
| WebView engine | WKWebView (macOS), CEF (Windows) |
| C++ standard | C++17 (`-std=c++17`) |

**Key JUCE modules used:**
- `juce_audio_processors` — AudioProcessor, APVTS, plugin hosting
- `juce_dsp` — ScopedNoDenormals, SmoothedValue
- `juce_gui_extra` — WebBrowserComponent (embedded WebView)
- `juce_audio_plugin_client` — VST3/AU wrapper generation

**Build flow:**

```
cmake --build build --target AudioPlugin02_All
  │
  ├─ BuildSvelteUI (custom target)
  │    npm install && npm run build   →   WebUI/dist/
  │
  ├─ juce_add_binary_data(WebResources)
  │    embeds WebUI/dist/* into binary
  │
  └─ post-build step
       copies Resources/ into .vst3 / .component bundle
```

The Svelte bundle is embedded directly in the plugin binary at link time, so the final `.vst3` or `.component` file is self-contained with no external web-server dependency.
