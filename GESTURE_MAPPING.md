# Gesture → Parameter Mapping Guide

How to add new hand gesture controls to `plugin-camera`.
All gesture logic lives in one file: `WebUI/src/components/GestureController.svelte`.

---

## 1. Architecture Overview

```
Camera.svelte
  └─ getUserMedia() → MediaStream
       │
       ├─► cameraStream store     (WebUI/src/stores/cameraStream.ts)
       │        │
       │        └─► GestureController.svelte
       │                 │  hidden <video> reads same stream
       │                 │  MediaPipe detectForVideo() every rAF tick
       │                 │
       │                 ├─► classifyGesture()  →  smooth()  →  gesture state
       │                 │        │
       │                 │        └─► params store  +  bridge.sendParameterChange()
       │                 │                 │                    │
       │                 │            Svelte UI             JUCE C++ DSP
       │                 │
       │                 └─► latestHands store  (WebUI/src/stores/landmarks.ts)
       │                              │
       └─────────────────────────────►  Camera.svelte canvas overlay (skeleton)
```

The bridge sends a **normalized 0–1 float** to JUCE via the `juce://setparameter` URL scheme.
JUCE maps it to the parameter's real range internally (e.g. 0–100 % for dryWet).
Everything in the JS side — stores, gestures, knobs — works exclusively in **normalized 0–1**.

---

## 2. All 31 Parameters

Every parameter ID is a key in `params` (store.ts) and a member of the `ParameterId` union type.

### 2a. Continuous knob parameters (send incremental float changes)

| ID | Display range | Mapping | Notes |
|----|--------------|---------|-------|
| `loCutFreq` | 50–18 000 Hz | skew 0.3 | logarithmic-ish |
| `hiCutFreq` | 50–18 000 Hz | skew 0.3 | |
| `hiCutQ` | 0.5–9.0 | skew 0.5 | |
| `erAmount` | 2–55 | skew 0.5 | |
| `erRate` | 0.07–1.3 Hz | skew 0.5 | |
| `erShape` | 0–1 | linear | |
| `crossoverFreq` | 200–8 000 Hz | skew 0.5 | |
| `diffusion` | 0–1 | linear | |
| `scale` | 0–1 | linear | |
| `decay` | 200–60 000 ms | skew 0.3 | logarithmic-ish |
| `damping` | 0–1 | linear | |
| `feedback` | 0–1 | linear | |
| `chorusAmount` | 0.01–4.0 | linear | |
| `chorusRate` | 0.01–8.0 Hz | skew 0.5 | |
| `reflectGain` | −30–+6 dB | linear | |
| `diffuseGain` | −30–+6 dB | linear | |
| `dryWet` | 0–100 % | linear | ← current gesture target |
| `predelay` | 0–500 ms | skew 0.5 | |
| `size` | 0.22–500 | log | `sizeToNormalized()` helper needed |
| `stereo` | 0–120° | linear | |

### 2b. Toggle parameters (send 0 or 1, not incremental)

| ID | Off | On |
|----|-----|----|
| `loCutEnabled` | filter bypassed | active |
| `hiCutEnabled` | filter bypassed | active |
| `erEnabled` | spin off | spin on |
| `chorusEnabled` | chorus off | on |
| `flatEnabled` | off | on |
| `cutEnabled` | off | on |
| `freeze` | normal | freeze tail |
| `highFilterType` | low-pass | shelving |

### 2c. Discrete selector parameters (send `index / (numOptions − 1)`)

| ID | Options | Normalized values |
|----|---------|------------------|
| `smooth` | Off / Low / Med / High | 0, 0.33, 0.67, 1.0 |
| `density` | Sparse / Low / Mid / High | 0, 0.33, 0.67, 1.0 |
| `reverbMode` | (not currently exposed in UI) | — |

---

## 3. Step-by-Step: Add a New Gesture Mapping

### Step 1 — Design the gesture shape

A gesture needs at least one unambiguous geometric property derived from the 21 hand landmarks.
Useful landmark indices (y increases downward in the frame):

```
 0  WRIST
 1  THUMB_CMC   2  THUMB_MCP   3  THUMB_IP    4  THUMB_TIP
 5  INDEX_MCP   6  INDEX_PIP   7  INDEX_DIP   8  INDEX_TIP
 9  MIDDLE_MCP 10  MIDDLE_PIP 11  MIDDLE_DIP 12  MIDDLE_TIP
13  RING_MCP   14  RING_PIP   15  RING_DIP   16  RING_TIP
17  PINKY_MCP  18  PINKY_PIP  19  PINKY_DIP  20  PINKY_TIP
```

Reliable geometric signals:
- **Finger extended**: `tip.y < pip.y` (tip above middle joint, hand upright)
- **Finger curled**: `tip.y > pip.y` (tip below middle joint)
- **Hand inverted**: `index_tip.y > wrist.y` (fingers below wrist)
- **Open palm**: all 4 non-thumb tips have `tip.y < mcp.y`
- **Fist**: all 4 non-thumb tips have `tip.y > pip.y`
- **Finger spread**: horizontal distance `|tip_a.x − tip_b.x|` is large
- **Hand height in frame**: `wrist.y` (0 = top, 1 = bottom)

All coordinates are **normalized 0–1** within the video frame.
Coordinates are from the **raw (unmirrored)** video data — do not flip x to account for the display mirror.

### Step 2 — Add a gesture type to `classifyGesture()`

Open `GestureController.svelte` and find `classifyGesture(lm: any[])`.
The function returns `'up' | 'down' | 'none'` today; extend the return type first:

```ts
// Change the return type union at the top of classifyGesture
function classifyGesture(lm: any[]): 'up' | 'down' | 'openPalm' | 'none' {
```

Then add a detection block before `return 'none'`:

```ts
// Example: open palm (all 4 fingers extended upward)
const allExtended =
    lm[8].y  < lm[6].y  &&   // index
    lm[12].y < lm[10].y &&   // middle
    lm[16].y < lm[14].y &&   // ring
    lm[20].y < lm[18].y;     // pinky

if (allExtended && lm[8].y < lm[0].y - 0.10) return 'openPalm';
```

### Step 3 — Add a smoothing history for the new gesture

In the state section near the top of the script:

```ts
let histOpenPalm = new Array<boolean>(SMOOTH_N).fill(false);
```

Then inside `smooth()`, add a third history and vote:

```ts
function smooth(raw: ReturnType<typeof classifyGesture>): typeof raw {
    histUp.shift();        histUp.push(raw === 'up');
    histDown.shift();      histDown.push(raw === 'down');
    histOpenPalm.shift();  histOpenPalm.push(raw === 'openPalm');

    if (histUp.filter(Boolean).length        >= SMOOTH_THRESHOLD) return 'up';
    if (histDown.filter(Boolean).length      >= SMOOTH_THRESHOLD) return 'down';
    if (histOpenPalm.filter(Boolean).length  >= SMOOTH_THRESHOLD) return 'openPalm';
    return 'none';
}
```

### Step 4 — Wire the gesture to a parameter

There are two interaction patterns:

#### Pattern A — Continuous (rate-based), for knobs

Increment or decrement the normalized value each frame while the gesture is held.

```ts
// In the detection loop, after `gesture = smooth(raw)`:
if (gesture === 'openPalm') {
    const cur  = get(params.decay);
    const next = cur + RATE;          // or -RATE to decrease
    if (next !== cur) {
        const clamped = Math.max(0, Math.min(1, next));
        setParameterValue('decay', clamped);
        bridge.sendParameterChange('decay', clamped);
    }
}
```

Rate tuning:
- `RATE = 0.006` → full 0–1 sweep in ~2.8 s at 60 fps
- `RATE = 0.003` → ~5.6 s (slow, precise)
- `RATE = 0.012` → ~1.4 s (fast, expressive)

For **perceptually log-scaled** parameters (decay, size, frequencies), consider applying the rate in log space so changes feel linear to the ear:

```ts
// Decay example: apply rate multiplicatively
const cur  = get(params.decay);
const next = cur * (1 + 0.02);   // +2% of current value per frame
```

#### Pattern B — Trigger (one-shot), for toggles and discrete selectors

Fire once when the gesture first appears (rising edge), not every frame.

```ts
let prevGesture: typeof gesture = 'none';

// ... at the end of the detection loop, after computing gesture:
if (gesture === 'openPalm' && prevGesture !== 'openPalm') {
    // Toggle freeze on the rising edge
    const cur = get(params.freeze);
    const next = cur > 0.5 ? 0 : 1;
    setParameterValue('freeze', next);
    bridge.sendParameterChange('freeze', next);
}
prevGesture = gesture;
```

For discrete selectors, cycle through options:

```ts
if (gesture === 'openPalm' && prevGesture !== 'openPalm') {
    const cur   = Math.round(get(params.density) * 3);  // 0–3
    const next  = (cur + 1) % 4;
    const norm  = next / 3;
    setParameterValue('density', norm);
    bridge.sendParameterChange('density', norm);
}
```

### Step 5 — Update the visual indicator

In the template section of `GestureController.svelte`, add a case for the new gesture inside the `{#if}` chain:

```svelte
{:else if gesture === 'openPalm'}
    <span class="arrow" title="Open palm">✋</span>
```

And update the label span:

```svelte
{gesture === 'up'        ? 'wet ↑'  :
 gesture === 'down'      ? 'dry ↑'  :
 gesture === 'openPalm'  ? 'freeze' : 'gesture'}
```

---

## 4. Technical Details

### 4a. Normalization

Every value crossing the JS↔JUCE bridge is **normalized 0–1**, regardless of the physical unit.

```
normalized  →  display
    0.0     →  min
    0.5     →  midpoint (not always perceptual center for skewed params)
    1.0     →  max
```

JUCE internally uses this formula for skewed parameters:
```
display = min + (max − min) × normalized^(1 / skew)
```

When a gesture needs to work in display units (e.g., "jump decay to 2 s"), convert first:
```ts
// decay: 200–60000 ms, skew 0.3
// display → normalized
const displayMs  = 2000;
const normalized = Math.pow((displayMs - 200) / 59800, 0.3);
```

Helpers for common conversions are exported from `state/store.ts`:
- `toNormalized(display, min, max, skew)` — generic skewed mapping
- `sizeToNormalized(display)` — special log mapping for the Size parameter

### 4b. Rate / Speed

The detection loop runs inside `requestAnimationFrame` — tied to screen refresh, typically **60 fps** but can be 30, 90, or 120 depending on the display.

To make the speed **frame-rate independent**, express rate as "per second" and scale by the measured frame delta:

```ts
let lastTime = performance.now();

function loop() {
    const now   = performance.now();
    const dt    = (now - lastTime) / 1000;   // seconds since last frame
    lastTime    = now;

    // ...detection...

    if (gesture === 'up') {
        const RATE_PER_SEC = 0.35;           // full range in ~2.9 s
        const delta = RATE_PER_SEC * dt;
        sendDryWet(get(params.dryWet) + delta);
    }
}
```

Typical reference rates:

| Effect | Rate/s | Approx. sweep time |
|--------|--------|--------------------|
| Very slow (precise automation) | 0.10 | 10 s |
| Slow (expressive) | 0.25 | 4 s |
| Default (dryWet today) | 0.36 | 2.8 s |
| Fast (dramatic) | 0.70 | 1.4 s |
| Instant snap | — | use trigger pattern (§ 4 above) |

### 4c. Smoothing

The current smoother is a **sliding window majority vote**:

```
SMOOTH_N         = 4   (window size in frames)
SMOOTH_THRESHOLD = 2   (minimum votes to confirm a gesture)
```

This introduces a latency of ~2–4 frames (33–66 ms at 60 fps) — imperceptible for hold gestures.

For **trigger gestures** (one-shot), you may want a stricter threshold (3/4 or 4/4) to avoid accidental fires, at the cost of slightly more latency.

For **continuous gestures** controlling fast parameters, reduce to 1/2 for near-instant response, but expect occasional false readings.

### 4d. Coordinate System

```
(0,0) ──────────────────► x (1,0)
  │     raw camera frame
  │     (NOT mirrored)
  │
  ▼
(0,1)                     (1,1)
```

- `x` increases to the **right** of the raw frame (user's left on a mirrored display)
- `y` increases **downward**
- `z` is approximate depth; positive z means closer to camera
- All values are **normalized to the bounding box of the detected hand**, not the full frame

Because the display mirror is CSS-only (`transform: scaleX(-1)`), landmark x coordinates are **not flipped** — the canvas overlay applies the same CSS mirror to stay aligned.

### 4e. Handedness

`results.handednesses[0][0].categoryName` returns `"Left"` or `"Right"`.
Avoid relying on this label: its meaning depends on whether the WebView (WKWebView in JUCE) pre-flips the camera stream before it reaches the `<video>` element. This is inconsistent across OS versions.
**Geometric checks** (landmark positions relative to each other) are always reliable.

### 4f. Multi-gesture conflicts

`classifyGesture` returns the **first matching branch** in order. If two gestures could overlap (e.g., "index up" and "all fingers up"), put the stricter check first. Alternatively, return a priority-ordered result array and handle conflicts in the loop.

---

## 5. Adding a Pose Gesture (Future — Body Landmarks)

The current setup uses `HandLandmarker` (hand only). To also detect body pose:

1. Add `PoseLandmarker` from `@mediapipe/tasks-vision` alongside `HandLandmarker` in `initMediaPipe()`.
2. Publish body results to a new store (`src/stores/pose.ts`).
3. Create a separate `classifyPose(poseLandmarks)` function.
4. Body landmark coordinates follow the same 0–1 normalized system, with 33 keypoints (nose, shoulders, elbows, wrists, hips, knees, ankles, etc.).

---

## 6. Quick-Reference Checklist

When adding a new gesture mapping:

- [ ] Design the gesture shape using landmark geometry (not handedness labels)
- [ ] Add a new return value to `classifyGesture()`'s return type
- [ ] Add a detection block inside `classifyGesture()` before `return 'none'`
- [ ] Add a history array `histXxx` and a vote inside `smooth()`
- [ ] Choose interaction pattern: **continuous** (rate × dt) or **trigger** (rising-edge)
- [ ] Use `get(params.yourParam)` to read the current value imperatively
- [ ] Call both `setParameterValue()` and `bridge.sendParameterChange()` with the same normalized 0–1 value
- [ ] Clamp the result to `[0, 1]` before sending
- [ ] Update the visual indicator in the template
- [ ] Rebuild: `npm run build` from `WebUI/`
