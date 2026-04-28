import { writable, derived } from 'svelte/store';
import type { ParameterId } from '../types/parameters';

// Per-parameter writable stores (normalized 0-1).
// Each store is independent — updating one param only notifies its own subscribers.
export const params = {
    // Input Filter
    loCutEnabled: writable(0),
    hiCutEnabled: writable(0),
    loCutFreq:    writable(0.35),
    hiCutFreq:    writable(0.42),
    hiCutQ:       writable(0.16),  // default 0.7071 → pow((0.7071-0.5)/8.5, 0.5) ≈ 0.156

    // Early Reflections
    erEnabled: writable(0),
    erAmount:  writable(0.3),
    erRate:    writable(0.3),
    erShape:   writable(0.5),

    // FDN Core
    reverbMode:    writable(0),
    crossoverFreq: writable(0.5),
    diffusion:     writable(0.6),
    scale:         writable(0.5),
    decay:         writable(0.42),
    damping:       writable(0.5),
    feedback:      writable(0.75),
    highFilterType: writable(0),   // 0 = low-pass, 1 = shelving

    // Chorus
    chorusEnabled: writable(0),
    chorusAmount: writable(0.05),  // default 0.2 in range 0.01–4.0 → (0.2-0.01)/3.99 ≈ 0.048
    chorusRate:   writable(0.27),

    // Output
    reflectGain: writable(0.8),
    diffuseGain: writable(0.8),
    dryWet:      writable(0.5),

    // Drive modules (Step 1+)
    gainDb:       writable(0.667),  // 0 dB: (-24 to +12) → 24/36 = 0.667
    outputTrimDb: writable(1.0),    // 0 dB: (-12 to 0)   → 12/12 = 1.0
    satDrive:     writable(0),      // 0-1, default off
    satMix:       writable(0),      // 0-100%, default 0
    satTone:      writable(1.0),    // 0-1, default 1.0 (bright)

    // Timbre filter (Step 3)
    timbreCutoff:    writable(0.76),   // 8000 Hz in 20–20000 Hz range, skew 0.3
    timbreResonance: writable(0.148),  // 0.7071 in 0.5–10 range, skew 0.5
    timbreDrivePre:  writable(0),      // 0 dB in 0–12 dB range

    // Vibrato (Step 4)
    vibratoRate:   writable(0.49),   // 2 Hz in 0.1–8 Hz range, skew 0.5
    vibratoDepth:  writable(0),      // 0–1, default off
    vibratoFadeIn: writable(0.447),  // 100 ms in 0–500 ms range, skew 0.5

    // Bottom Utility Row
    predelay: writable(0.08),
    smooth:   writable(0),
    size:     writable(0.5),
    freeze:   writable(0),
    flatEnabled: writable(0),
    cutEnabled:  writable(0),
    stereo:   writable(1.0),  // default 120 degrees → normalized 120/120 = 1.0
    density:  writable(1.0),  // default High (index 3 → normalized 1.0) = 4 active stages
};

export function setParameterValue(id: ParameterId, value: number) {
    (params as Record<string, { set: (v: number) => void }>)[id]?.set(value);
}

// ---------------------------------------------------------------------------
// Inverse mapping utilities (display → normalized), used by NeuNumber components.
// ---------------------------------------------------------------------------

// General: skewed APVTS parameters
// normalized = ((display - min) / (max - min)) ^ skew
export function toNormalized(
    display: number, min: number, max: number, skew: number
): number {
    return Math.pow((display - min) / (max - min), skew);
}

// Special: size uses a log display mapping (stored as 0–1 in APVTS, shown as 0.22–500)
// normalized = log(display / 0.22) / log(500 / 0.22)
export function sizeToNormalized(display: number): number {
    return Math.log(display / 0.22) / Math.log(500.0 / 0.22);
}

// ---------------------------------------------------------------------------
// Selector indices — only recompute when their own param changes
// ---------------------------------------------------------------------------
export const modeSelected        = derived(params.reverbMode,    $v => Math.round($v));
export const smoothSelected      = derived(params.smooth,        $v => Math.round($v * 3));
export const densitySelected     = derived(params.density,       $v => Math.round($v * 3));
export const flatEnabledSelected = derived(params.flatEnabled,   $v => Math.round($v));
export const cutEnabledSelected  = derived(params.cutEnabled,    $v => Math.round($v));

// ---------------------------------------------------------------------------
// Display values — denormalized using JUCE skewed formula:
//   display = min + (max - min) * normalizedValue ^ (1 / skew)
// For linear params (skew=1) this is standard lerp.
// ---------------------------------------------------------------------------

// loCutFreq: 50–18000 Hz, skew=0.3
export const loCutHz = derived(params.loCutFreq,
    $v => Math.round(50 + 17950 * Math.pow($v, 1.0 / 0.3)));

// hiCutFreq: 50–18000 Hz, skew=0.3
export const hiCutHz = derived(params.hiCutFreq,
    $v => Math.round(50 + 17950 * Math.pow($v, 1.0 / 0.3)));

// hiCutQ: 0.5–9.0, skew=0.5
export const hiCutQDisplay = derived(params.hiCutQ,
    $v => (0.5 + 8.5 * Math.pow($v, 1.0 / 0.5)).toFixed(2));

// erAmount: 2.0–55.0, skew=0.5
export const erAmountDisplay = derived(params.erAmount,
    $v => (2.0 + 53.0 * Math.pow($v, 1.0 / 0.5)).toFixed(1));

// erRate: 0.07–1.3 Hz, skew=0.5
export const erRateDisplay = derived(params.erRate,
    $v => (0.07 + 1.23 * Math.pow($v, 1.0 / 0.5)).toFixed(2));

// reflectGain: −30–6 dB, skew=1.0 (linear)
export const reflectGainDb = derived(params.reflectGain,
    $v => (-30.0 + 36.0 * $v).toFixed(1));

// diffuseGain: −30–6 dB, skew=1.0 (linear)
export const diffuseGainDb = derived(params.diffuseGain,
    $v => (-30.0 + 36.0 * $v).toFixed(1));

// decay: 200–60000 ms, skew=0.3
export const decayMs = derived(params.decay,
    $v => Math.round(200 + 59800 * Math.pow($v, 1.0 / 0.3)));

// predelay: 0–500 ms, skew=0.5
export const predelayMs = derived(params.predelay,
    $v => (500.0 * Math.pow($v, 1.0 / 0.5)).toFixed(1));

// stereo: 0–120 degrees, skew=1.0 (linear)
export const stereoDeg = derived(params.stereo,
    $v => Math.round(120.0 * $v));

// dryWet: 0–100 %, skew=1.0 (linear)
export const dryWetPct = derived(params.dryWet,
    $v => Math.round(100.0 * $v));

// chorusAmount: 0.01–4.0, skew=1.0 (linear)
export const chorusAmountDisplay = derived(params.chorusAmount,
    $v => (0.01 + 3.99 * $v).toFixed(2));

// chorusRate: 0.01–8.0 Hz, skew=0.5
export const chorusRateDisplay = derived(params.chorusRate,
    $v => (0.01 + 7.99 * Math.pow($v, 1.0 / 0.5)).toFixed(2));

// size: 0–1 stored, log display mapping → 0.22–500
export const sizeDisplay = derived(params.size,
    $v => (0.22 * Math.pow(500.0 / 0.22, $v)).toFixed(2));

// crossoverFreq: 200–8000 Hz, skew=0.5 (kept for SVG node / UI use)
export const crossoverHz = derived(params.crossoverFreq,
    $v => Math.round(200 + 7800 * Math.pow($v, 1.0 / 0.5)));

// diffusion: 0–1, linear (kept for percentage display)
export const diffusionPct = derived(params.diffusion,
    $v => ($v * 100).toFixed(0));

// ---------------------------------------------------------------------------
// SVG node positions — only recompute when their own param changes
// ---------------------------------------------------------------------------
export const svgNode1X = derived(params.crossoverFreq, $v => 80  + $v * 40);
export const svgNode1Y = derived(params.damping,       $v => 15  + $v * 40);
export const svgNode2X = derived(params.feedback,      $v => 140 + $v * 30);
export const svgNode2Y = derived(params.diffusion,     $v => 40  + $v * 20);
