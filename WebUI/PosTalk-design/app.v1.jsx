/* PosTalk · main app */

const { useState, useEffect } = React;

// ── Initial parameter values (mirrors WebUI/src/state/store.ts defaults) ──
const INITIAL_PARAMS = {
  loCutEnabled: 0, hiCutEnabled: 0,
  loCutFreq: 0.35, hiCutFreq: 0.42, hiCutQ: 0.16,
  erEnabled: 0, erAmount: 0.3, erRate: 0.3, erShape: 0.5,
  reverbMode: 0, crossoverFreq: 0.5, diffusion: 0.6, scale: 0.5,
  decay: 0.42, damping: 0.5, feedback: 0.75, highFilterType: 0,
  chorusEnabled: 0, chorusAmount: 0.05, chorusRate: 0.27,
  reflectGain: 0.8, diffuseGain: 0.8, dryWet: 0.5,
  gainDb: 0.667, outputTrimDb: 1.0,
  satDrive: 0.35, satMix: 0.6, satTone: 0.5,
  timbreCutoff: 0.76, timbreResonance: 0.148, timbreDrivePre: 0,
  vibratoRate: 0.49, vibratoDepth: 0.2, vibratoFadeIn: 0.447,
  pitchFrequency: 0.04, pitchOctaveStep: 0,
  sustainEnabled: 0, cutNow: 0,
  predelay: 0.08, smooth: 0, size: 0.5,
  freeze: 0, flatEnabled: 0, cutEnabled: 0,
  stereo: 1.0, density: 1.0
};

const PRESET_NAMES = ['Abyss', 'Chamber', 'Room', 'Studio', 'Hall', 'Infinity', 'IPA'];

const PRESETS = [
{ decay: 0.22, damping: 0.90, crossoverFreq: 0.16, size: 0.20, diffusion: 0.30, feedback: 0.72, dryWet: 0.60, reverbMode: 1, scale: 0.25, density: 0, satDrive: 0.30, satMix: 0.35, satTone: 0.25, timbreCutoff: 0.38, predelay: 0.10, stereo: 0.33 },
{ decay: 0.32, damping: 0.72, crossoverFreq: 0.28, size: 0.35, diffusion: 0.50, feedback: 0.75, dryWet: 0.50, reverbMode: 1, scale: 0.35, density: 0.33, satDrive: 0.20, satMix: 0.20, satTone: 0.40, timbreCutoff: 0.54, predelay: 0.155, stereo: 0.46, erEnabled: 1, erAmount: 0.55 },
{ decay: 0.35, damping: 0.55, crossoverFreq: 0.48, size: 0.48, diffusion: 0.60, feedback: 0.78, dryWet: 0.45, reverbMode: 1, scale: 0.50, density: 0.67, satDrive: 0.10, satMix: 0.10, satTone: 0.60, timbreCutoff: 0.66, predelay: 0.20, stereo: 0.54, erEnabled: 1, erAmount: 0.66 },
{ decay: 0.40, damping: 0.38, crossoverFreq: 0.70, size: 0.55, diffusion: 0.68, feedback: 0.82, dryWet: 0.40, reverbMode: 0, scale: 0.55, density: 1, satTone: 0.80, timbreCutoff: 0.76, predelay: 0.237, stereo: 0.63, erEnabled: 1, erAmount: 0.79, chorusEnabled: 1, chorusAmount: 0.15 },
{ decay: 0.50, damping: 0.20, crossoverFreq: 0.86, size: 0.72, diffusion: 0.75, feedback: 0.85, dryWet: 0.35, reverbMode: 0, scale: 0.70, density: 1, satTone: 1.0, timbreCutoff: 0.86, predelay: 0.265, stereo: 0.75, erEnabled: 1, erAmount: 0.90, chorusEnabled: 1, chorusAmount: 0.20 },
{ decay: 0.77, damping: 0.08, crossoverFreq: 0.97, size: 0.90, diffusion: 0.85, feedback: 0.90, dryWet: 0.28, reverbMode: 0, scale: 0.90, density: 1, satTone: 1.0, timbreCutoff: 0.94, predelay: 0.316, stereo: 0.92, erEnabled: 1, erAmount: 0.98, chorusEnabled: 1, chorusAmount: 0.25, vibratoDepth: 0.15, pitchFrequency: 0.218, pitchOctaveStep: 1 },
{ decay: 0.77, damping: 0.08, crossoverFreq: 0.97, size: 0.90, diffusion: 0.85, feedback: 0.90, dryWet: 0.98, reverbMode: 0, scale: 0.90, density: 1, satTone: 1.0, timbreCutoff: 0.94, predelay: 0.316, stereo: 0.92, erEnabled: 1, erAmount: 0.98, chorusEnabled: 1, chorusAmount: 0.25, hiCutEnabled: 1, hiCutFreq: 0.35, vibratoRate: 1.0, vibratoDepth: 0.15 }];


// ── Accent color presets ──
const ACCENT_PRESETS = {
  amber: { accent: '#ffb648', accent2: '#ffb648' },
  orange: { accent: '#ff7a2c', accent2: '#ff7a2c' },
  teal: { accent: '#22d3c0', accent2: '#22d3c0' },
  dual: { accent: '#ff7a2c', accent2: '#22d3c0' }
};

// ── Tweak defaults ──
const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "accent": "amber",
  "density": "normal"
} /*EDITMODE-END*/;

function App() {
  const [params, setParams] = useState(INITIAL_PARAMS);
  const [activePreset, setActivePreset] = useState(null);
  const [pwr, setPwr] = useState(true);
  const [bypass, setBypass] = useState(false);
  const [mono, setMono] = useState(false);
  const [gesture, setGesture] = useState(null);

  const [tweaks, setTweak] = useTweaks(TWEAK_DEFAULTS);

  // Apply tweak: accent color
  useEffect(() => {
    const root = document.documentElement;
    const a = ACCENT_PRESETS[tweaks.accent] || ACCENT_PRESETS.amber;
    root.style.setProperty('--accent', a.accent);
    root.style.setProperty('--accent-2', a.accent2);
    root.style.setProperty('--accent-dim', a.accent + '6c');
    root.style.setProperty('--accent-soft', a.accent + '2a');
    root.style.setProperty('--accent-glow', a.accent + '8c');
    root.dataset.density = tweaks.density;
  }, [tweaks]);

  const set = (key, val) => setParams((p) => ({ ...p, [key]: val }));

  const applyPreset = (idx) => {
    setActivePreset(idx);
    setParams((p) => ({ ...p, ...PRESETS[idx] }));
  };

  // ── simulate gesture mapping by hover on the gmap rows ──
  useEffect(() => {
    if (gesture === 'up') set('sustainEnabled', 1);else
    if (gesture === 'down') set('sustainEnabled', 0);
  }, [gesture]);

  return (
    <div className={`plugin ${bypass ? 'bypassed' : ''}`}>
      {/* ── Header ── */}
      <div className="header">
        <div className="brand">
          <span className="brand-name" style={{ fontFamily: "\"Josefin Sans\"" }}>POSTALK</span>
          <span className="brand-sep">—</span>
          <span className="brand-sub">DSP · 01 · REVERB</span>
        </div>
        <div className="header-controls">
          <Lamp label="PWR" on={pwr} onClick={() => setPwr(!pwr)} />
          <Lamp label="Bypass" on={bypass} onClick={() => setBypass(!bypass)} />
          <Lamp label="Mono" on={mono} onClick={() => setMono(!mono)} />
        </div>
      </div>

      <div className="body">
        {/* ── Top row ── */}
        <div className="row-top">

          {/* Presets */}
          <div className="panel">
            <div className="panel-title">Presets</div>
            <div className="presets-grid">
              {PRESET_NAMES.map((name, i) =>
              <button
                key={i}
                className={`preset-btn ${activePreset === i ? 'active' : ''}`}
                onClick={() => applyPreset(i)}
                title={name}>
                
                  {i + 1}
                </button>
              )}
            </div>
            <div className="preset-name">
              {activePreset !== null ? PRESET_NAMES[activePreset] : '— select —'}
            </div>
          </div>

          {/* Camera hero */}
          <CameraCard activeGesture={gesture} />

          {/* Gesture map */}
          <div className="panel">
            <GestureMap activeGesture={gesture} onSimulate={setGesture} />
          </div>
        </div>

        {/* ── Bottom row · 4 panels ── */}
        <div className="row-bottom">
          <ReverbPanel p={params} set={set} />
          <SaturationPanel p={params} set={set} />
          <TimbrePanel p={params} set={set} />
          <ShimmerPanel p={params} set={set} />
        </div>

        {/* ── Footer ── */}
        <div className="footer">
          <span>postalk · spectral processor · v0.1</span>
          <div className="footer-right">
            <span>{bypass ? 'bypassed' : pwr ? 'active' : 'standby'}</span>
            <span className="pulse-dot" />
          </div>
        </div>
      </div>

      {/* ── Tweaks ── */}
      <TweaksPanel title="Tweaks">
        <TweakSection label="Accent">
          <TweakRadio
            value={tweaks.accent}
            options={[
            { value: 'amber', label: 'Amber' },
            { value: 'orange', label: 'Orange' },
            { value: 'teal', label: 'Teal' },
            { value: 'dual', label: 'Dual' }]
            }
            onChange={(v) => setTweak('accent', v)} />
          
        </TweakSection>
        <TweakSection label="Density">
          <TweakRadio
            value={tweaks.density}
            options={[
            { value: 'compact', label: 'Compact' },
            { value: 'normal', label: 'Normal' },
            { value: 'spacious', label: 'Spacious' }]
            }
            onChange={(v) => setTweak('density', v)} />
          
        </TweakSection>
      </TweaksPanel>
    </div>);

}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);