# PosTalk DSP Modules Technical Audit

## Scope
This audit reviews the DSP modules listed in `ARCHITECTURE.md` and reframes each module as an engineering recommendation grounded in established reverberation and plugin-design practice. The goal is to make the framework read as a professional, evidence-based audio product rather than a feature list.

## 1. Input and Gain Staging

### GainModule
Purpose: manage input gain and output trim with stable real-time behavior.

Good practices:
- Use smoothed gain changes to prevent zipper noise.
- Keep headroom before nonlinear stages.
- Apply input trim before any saturation or tone shaping.
- Avoid denormal issues by enabling denormal protection in the audio thread.

Parameter guidance:
- `gainDb`: map linearly in dB, not linearly in amplitude.
- `outputTrimDb`: use the same smoothing strategy as input gain.

### Saturation
Purpose: add controlled harmonic color before the reverb core.

Good practices:
- Use a soft clipper or `tanh`-style shaper for gentle coloration.
- Pair the shaper with anti-aliasing, ideally oversampling or ADAA.
- Keep drive ranges conservative so the reverb input does not collapse dynamically.
- Normalize output level to preserve A/B comparability.

Parameter guidance:
- `satDrive`: scale perceptually; small value changes should matter most near the clean-to-warm transition.
- `satMix`: use equal-power wet/dry blending if parallel saturation is enabled.
- `satTone`: implement as a stable post-shaper tilt or low-pass control.

### TimbreFilter
Purpose: provide tone shaping before the diffuse network.

Good practices:
- Use a modulation-safe filter design for automation.
- Prefer TPT/ZDF structures when filter parameters may move at audio rate.
- Clamp cutoff away from Nyquist to preserve stability.
- Use double precision in coefficient/state calculations when feasible.

Parameter guidance:
- `timbreCutoff`: map logarithmically or with a skewed curve.
- `timbreResonance`: limit resonance to stable values and smooth changes.
- `timbreDrivePre`: apply before the filter only if the tonal goal requires it.

### Vibrato
Purpose: introduce slow delay variation for thickness and motion.

Good practices:
- Keep vibrato depth small to avoid audible detuning.
- Use decorrelated modulation where multiple channels or lines are involved.
- Smooth depth changes, especially when enabling the effect.
- Use interpolation that is efficient and stable for fractional delay reads.

Parameter guidance:
- `vibratoRate`: use a musical low-frequency range; logarithmic control spacing is helpful.
- `vibratoDepth`: scale conservatively and reset gracefully when the parameter crosses zero.
- `vibratoFadeIn`: ramp depth in over a short time to avoid discontinuities.

## 2. Input Conditioning

### InputFilter
Purpose: remove unwanted low-end rumble and excessive top-end energy before the reverb.

Good practices:
- Use stable, coefficient-smoothed filters for cutoff automation.
- Keep low-cut and high-cut options independent when possible.
- Use one consistent frequency scaling model across the plugin.
- Prevent phase surprises by choosing known filter topologies.

Parameter guidance:
- `loCutEnabled` and `hiCutEnabled`: store as booleans with immediate but click-free updates.
- `loCutFreq` and `hiCutFreq`: use log mapping.
- `hiCutQ`: keep Q within a range that remains musically useful but stable.
- `predelay`: allow enough range for room-distance simulation while smoothing changes.

### Predelay
Purpose: separate direct sound from the reverberant field.

Good practices:
- Use a clean delay line with fractional support if needed.
- Smooth changes to avoid clicks during automation.
- Keep the maximum value long enough for hall-style spaces but not so large that it becomes an obvious echo unless intended.
- Maintain stereo consistency when using dual-channel predelay.

Parameter guidance:
- `predelay`: map in milliseconds with a perceptual skew so smaller values have finer resolution.

## 3. Early Reflections and Diffusion

### EarlyReflections
Purpose: create the first spatial cues that establish room size and source position.

Good practices:
- Use a small set of well-chosen taps or geometric-style reflections.
- Vary tap gain and delay to avoid a synthetic machine-gun pattern.
- Keep the early pattern stable enough to be recognizable, but not so rigid that it sounds artificial.
- Allow early reflections to feed the late tail smoothly rather than abruptly.

Parameter guidance:
- `erEnabled`: bypass cleanly.
- `erAmount`: control the balance with a perceptual mix law.
- `erRate`: if modulation is used, keep it subtle and decorrelated.
- `erShape`: map to discrete reflection patterns or a continuous density morph.
- `reflectGain`: use a calibrated gain range, not an unrestricted boost.

### DiffusionNetwork
Purpose: increase echo density before the main FDN tail.

Good practices:
- Use allpass diffusion sparingly and intentionally.
- Avoid too many short allpass stages, which can cause ringing or coloration.
- Keep diffusion stages low-cost and stable.
- Verify that diffusion improves density without making the onset overly smeared.

Parameter guidance:
- `diffusion`: use a nonlinear control curve so low-to-mid ranges are easier to tune.
- `diffuseGain`: keep within a bounded range to avoid over-coloration.

### DiffusionStage
Purpose: provide an additional diffusion layer for smoothing or pre-tail thickening.

Good practices:
- Make the stage optional or mode-dependent so it does not stack unnecessarily with other diffusers.
- Use stable allpass or decorrelator structures.
- Check coloration against transient material such as snare and piano.

Parameter guidance:
- If exposed, keep controls simple: density, feedback, and amount are usually enough.

## 4. Late Reverb Core

### FDNReverb
Purpose: generate the dense, diffuse late reverberation tail.

Good practices:
- Use an orthogonal or unitary feedback matrix for a lossless prototype.
- Choose delay lengths that avoid repeated relationships and modal clustering.
- Keep per-line processing deterministic and allocation-free.
- Separate decay control from tonal shaping.
- Ensure the tail remains stable under automation and modulation.

Parameter guidance:
- `reverbMode`: use discrete modes if they correspond to clear topology changes.
- `decay`: map nonlinearly so the most useful musical range has finer resolution.
- `size`: control average delay scale and perceived room scale with a log or skewed mapping.
- `freeze`: hold the internal energy safely without runaway growth.
- `feedback`: clamp to safe values and document how it interacts with decay.
- `density`: if exposed, connect it to diffusion/modulation behavior rather than raw gain.

### FeedbackMatrix
Purpose: redistribute energy among delay lines while preserving mixing quality.

Good practices:
- Prefer normalized Hadamard mixing for efficient dense coupling in power-of-two sizes.
- Keep matrix implementation branch-free or low-branch for real-time safety.
- Validate energy conservation numerically.
- Avoid matrices that collapse mixing quality at larger orders unless there is a deliberate artistic reason.

Parameter guidance:
- If the matrix is not user-facing, keep it fixed and document the rationale.
- If selectable, expose only a small number of meaningful options.

### DelayLine8
Purpose: store the delayed states that form the modal structure of the FDN.

Good practices:
- Use mutually prime or otherwise well-spaced base lengths.
- Avoid linear or arithmetic delay spacing that leads to audible repetition.
- Use interpolation that is stable and sufficiently transparent for modulation.
- Preallocate all buffers.

Parameter guidance:
- Base lengths should not be directly user-editable unless in an advanced mode.
- If tunable, present them as room-size or scale controls rather than raw samples.

### CrossoverFilter8
Purpose: apply frequency-dependent damping inside the feedback loop.

Good practices:
- Use separate low-pass and high-pass shaping as needed for realistic decay behavior.
- Prefer stable filter structures for any modulated parameters.
- Keep left and right or per-line behavior consistent enough that the image does not wobble.
- Validate that the filter does not shift the apparent tuning of the tail.

Parameter guidance:
- `crossoverFreq`: use logarithmic scaling.
- `hiCutQ`: keep conservative unless resonant character is specifically desired.

## 5. Motion and Space

### SpinModulator
Purpose: add slow motion and decorrelate the tail.

Good practices:
- Keep modulation depth low enough that pitch drift remains subtle.
- Use independent but related LFOs across channels or lines.
- Ensure the average pitch shift across the network stays near neutral.
- Smooth modulator changes and avoid abrupt resets.

Parameter guidance:
- If exposed, mod speed should be in a slow range optimized for texture, not vibrato.
- Depth should default to subtle values.

### Chorus
Purpose: thicken the output and add stereo movement.

Good practices:
- Treat chorus as a coloration stage, not a required part of the reverb core.
- Use interpolation and delay modulation that remain stable under automation.
- Keep the wet chorus return controlled so it does not dominate the tail.

Parameter guidance:
- `chorusEnabled`: bypass should be click-free.
- `chorusAmount`: use a restrained range.
- `chorusRate`: map logarithmically.

### StereoWidener
Purpose: widen the stereo image while preserving mono compatibility.

Good practices:
- Use mid-side processing with low-frequency protection on the side channel.
- Avoid extreme phase tricks that collapse badly in mono.
- Monitor correlation internally if width can be pushed hard.
- Keep the widening stage after the diffuse core or output shaping.

Parameter guidance:
- `stereo`: map so the neutral center position is easy to hit.
- `width`: if user-facing, constrain the maximum range to avoid phase cancellation.

### DryWetMixer
Purpose: combine dry source and processed reverb output smoothly.

Good practices:
- Use equal-power blending for perceptual consistency.
- Smooth the crossfade to avoid zipper noise.
- Preserve the dry signal path if latency compensation or A/B comparison is needed.
- Ensure full wet and full dry states behave predictably.

Parameter guidance:
- `dryWet`: use an equal-power curve rather than linear amplitude mixing.

## 6. Freeze and State Control

### Freeze
Purpose: sustain the internal reverberant state for ambient or creative effects.

Good practices:
- Prevent gain runaway when freeze is engaged.
- Crossfade into and out of freeze mode smoothly.
- Keep modulation behavior defined during freeze so the effect remains musically useful.
- Document whether input energy is blocked or simply recycled.

Parameter guidance:
- `freeze`: toggle should be debounced and state-safe.
- If linked to feedback, constrain the internal loop to a controlled energy level.

## 7. Control and Smoothing

### SmoothedValue and parameter handling
Purpose: prevent audible steps from UI or automation changes.

Good practices:
- Use smoothing on every parameter that can affect gain, frequency, delay, or modulation.
- Choose smoothing times based on the control type, not a single universal value.
- Use block- or sample-rate smoothing consistently.
- Keep parameter conversion centralized so UI and DSP stay aligned.

Parameter guidance:
- Continuous parameters such as decay, cutoff, mix, and width should be smoothed.
- Boolean parameters should switch cleanly but may still need state-safe transitions.

### APVTS and parameter mapping
Purpose: expose all plugin controls in a host-compatible, automatable way.

Good practices:
- Use normalized 0–1 storage with perceptual mapping in the DSP/UI layers.
- Keep IDs stable across versions.
- Define ranges and defaults that match musical expectations.
- Avoid exposing too many raw technical controls to regular users.

Parameter guidance:
- `gainYAmt`, `cutoffYAmt`, `satDriveYAmt`, `satMixYAmt`, `vibratoDepthYAmt`, `dryWetYAmt`, `decayYAmt`, `dampingYAmt`: treat as macro-routing amounts and keep their ranges intuitive.
- `macroY`: use as a high-level performance macro with clear defaults.
- `gestureModeToggle`: keep as an explicit mode switch with safe fallback behavior.

## 8. Output and Presentation

### Output stage
Purpose: present the final reverb without clipping or tonal imbalance.

Good practices:
- Add output limiting only if absolutely necessary and keep it transparent.
- Level-match major mode changes so A/B comparisons stay fair.
- Ensure bypass is true bypass or musically equivalent, depending on architecture.
- Keep metering accurate enough to reveal clipping and correlation issues.

Parameter guidance:
- `outputTrimDb`: smooth changes and preserve usable headroom.

## 9. Gesture and UI Pipeline

### Camera and gesture control
Purpose: map human motion to macro control in a stable, musical way.

Good practices:
- Treat gesture input as a control source, not direct sample-accurate DSP input.
- Filter and normalize gesture data before mapping it to audio parameters.
- Provide a clear fallback if camera input is unavailable.
- Prevent rapid gesture jitter from modulating multiple controls at once.

Parameter guidance:
- Gesture-mapped parameters should have per-target scaling and smoothing.
- Macro control should not override manual edits without explicit user intent.

## 10. Recommended wording for the final audit
Use phrasing such as:
- “This module follows common professional DSP practice by...”
- “This choice is appropriate because it reduces...”
- “This parameter is best mapped with a perceptual curve to...”
- “This stage should remain modulation-safe to preserve stability under automation.”

Avoid phrasing such as:
- “This is the only correct method.”
- “This guarantees perfection.”
- “This is always superior in all contexts.”

## 11. Module coverage checklist
- GainModule
- Saturation
- TimbreFilter
- Vibrato
- InputFilter
- Predelay
- EarlyReflections
- DiffusionNetwork
- DiffusionStage
- FDNReverb
- FeedbackMatrix
- DelayLine8
- CrossoverFilter8
- SpinModulator
- Chorus
- StereoWidener
- DryWetMixer
- Freeze
- APVTS parameter system
- SmoothedValue parameter smoothing
- Gesture and WebView control pipeline
- Output trim and final presentation

## 12. Closing recommendation
The architecture is strongest when each module is described as a controlled, real-time-safe building block that serves a specific acoustic or usability purpose. The most professional version of the audit will connect each parameter to a perceptual or stability reason, then cite the corresponding DSP principle or source.
