export type ParameterId =
    // Input Filter (5)
    | 'loCutEnabled' | 'hiCutEnabled' | 'loCutFreq' | 'hiCutFreq' | 'hiCutQ'
    // Early Reflections (4)
    | 'erEnabled' | 'erAmount' | 'erRate' | 'erShape'
    // FDN Core (8)
    | 'reverbMode' | 'crossoverFreq' | 'diffusion' | 'scale'
    | 'decay' | 'damping' | 'feedback' | 'highFilterType'
    // Chorus (3)
    | 'chorusEnabled' | 'chorusAmount' | 'chorusRate'
    // Output (3)
    | 'reflectGain' | 'diffuseGain' | 'dryWet'
    // Bottom Utility Row (8)
    | 'predelay' | 'smooth' | 'size' | 'freeze'
    | 'flatEnabled' | 'cutEnabled' | 'stereo' | 'density'
    // Module 1 — Gain (2)
    | 'gainDb' | 'outputTrimDb'
    // Module 2 — Timbre (3)
    | 'timbreCutoff' | 'timbreResonance' | 'timbreDrivePre'
    // Module 3 — Saturation (3)
    | 'satDrive' | 'satMix' | 'satTone'
    // Module 5 — Modulation (3)
    | 'vibratoRate' | 'vibratoDepth' | 'vibratoFadeIn'
    // Module 6 — Pitch Shifter (2)
    | 'pitchFrequency' | 'pitchOctaveStep'
    // Performance controls (2)
    | 'sustainEnabled' | 'cutNow'
    // Stage 2 — Macro Y (1)
    | 'macroY'
    // Y Amount — modulation depth per parameter (8)
    | 'gainYAmt' | 'cutoffYAmt' | 'satDriveYAmt' | 'satMixYAmt'
    | 'vibratoDepthYAmt' | 'dryWetYAmt' | 'decayYAmt' | 'dampingYAmt'
    // Gesture mode toggle (1)
    | 'gestureModeToggle';
// Total: 56 unique IDs, matching ParameterIDs.h exactly.

export interface PluginParameter {
    id: ParameterId;
    name: string;
    value: number; // normalized 0-1 (matches JUCE param->getValue())
    min: number;
    max: number;
    defaultValue: number;
}

export type ParameterState = Record<ParameterId, PluginParameter>;
