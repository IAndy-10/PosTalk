/* PosTalk · the four parameter panels — Reverb, Saturation, Timbre, Shimmer */

const { useState } = React;

// helpers to display normalized values like the Svelte code
const dl = (n, mn, mx) => mn + (mx - mn) * n;
const ds = (n, mn, mx, skew) => mn + (mx - mn) * Math.pow(Math.max(0, n), 1 / skew);

const fmt = (v, decimals = 2, unit = '') => `${v.toFixed(decimals)}${unit}`;

// ─── REVERB panel ──────────────────────────────────────────────────
function ReverbPanel({ p, set }) {
  return (
    <div className="panel scroll" style={{ gridColumn: 'span 1' }}>
      <div className="panel-title">Reverb</div>

      <div className="group-label">Performance</div>
      <div className="perf-row">
        <LedToggle
          label="Sustain"
          active={p.sustainEnabled > 0.5}
          onClick={() => set('sustainEnabled', p.sustainEnabled > 0.5 ? 0 : 1)} />
        
        <button className="cut-btn" onMouseDown={() => set('cutNow', 1)} onMouseUp={() => set('cutNow', 0)}>
          Cut
        </button>
      </div>

      <div className="group-label">Core</div>
      <div className="knob-row">
        <Knob label="Decay" value={p.decay} onChange={(v) => set('decay', v)}
        display={fmt(ds(p.decay, 200, 60000, 0.3), 0, ' ms')} size="md" />
        <Knob label="Diffusion" value={p.diffusion} onChange={(v) => set('diffusion', v)}
        display={fmt(p.diffusion, 2)} size="md" />
        <Knob label="Size" value={p.size} onChange={(v) => set('size', v)}
        display={fmt(p.size, 2)} size="md" />
        <Knob label="Damping" value={p.damping} onChange={(v) => set('damping', v)}
        display={fmt(p.damping, 2)} size="md" />
        <Knob label="Feedback" value={p.feedback} onChange={(v) => set('feedback', v)}
        display={fmt(p.feedback, 2)} size="md" />
        <Knob label="Stereo" value={p.stereo} onChange={(v) => set('stereo', v)}
        display={fmt(dl(p.stereo, 0, 120), 0, '°')} size="md" />
      </div>

      <div className="group-label">Mix</div>
      <div className="knob-row">
        <Knob label="Dry/Wet" value={p.dryWet} onChange={(v) => set('dryWet', v)}
        display={fmt(dl(p.dryWet, 0, 100), 0, '%')} size="md" />
        <NumberField label="Predelay" value={p.predelay} onChange={(v) => set('predelay', v)}
        display={fmt(ds(p.predelay, 0, 500, 0.5), 1, ' ms')} />
        <NumberField label="Reflect" value={p.reflectGain} onChange={(v) => set('reflectGain', v)}
        display={fmt(dl(p.reflectGain, -30, 6), 1, ' dB')} />
        <NumberField label="Diffuse" value={p.diffuseGain} onChange={(v) => set('diffuseGain', v)}
        display={fmt(dl(p.diffuseGain, -30, 6), 1, ' dB')} />
      </div>

      <div className="group-label">FDN</div>
      <div className="knob-row">
        <Knob label="Crossover" value={p.crossoverFreq} onChange={(v) => set('crossoverFreq', v)}
        display={fmt(ds(p.crossoverFreq, 200, 8000, 0.5), 0, ' Hz')} size="sm" />
        <Knob label="Scale" value={p.scale} onChange={(v) => set('scale', v)}
        display={fmt(p.scale, 2)} size="sm" />
        <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
          <div className="numf-label">Mode</div>
          <div style={{ display: 'flex', gap: 4 }}>
            <LedToggle label="A" active={p.reverbMode < 0.5} onClick={() => set('reverbMode', 0)} />
            <LedToggle label="B" active={p.reverbMode >= 0.5} onClick={() => set('reverbMode', 1)} />
          </div>
        </div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
          <div className="numf-label">Density</div>
          <div style={{ display: 'flex', gap: 3 }}>
            {[0, 1, 2, 3].map((i) =>
            <LedToggle key={i} label={String(i)}
            active={Math.round(p.density * 3) === i}
            onClick={() => set('density', i / 3)} />
            )}
          </div>
        </div>
      </div>
      <div className="num-row" style={{ marginTop: 8 }}>
        <NumberField label="Smooth" value={p.smooth} onChange={(v) => set('smooth', v)}
        display={String(Math.round(p.smooth * 3))} />
        <NumberField label="HF Filter" value={p.highFilterType} onChange={(v) => set('highFilterType', v)}
        display={p.highFilterType < 0.5 ? 'LP' : 'SH'} />
      </div>

      <div className="group-label">Gates</div>
      <div style={{ display: 'flex', gap: 6, flexWrap: 'wrap' }}>
        <LedToggle label="Freeze" active={p.freeze > 0.5} onClick={() => set('freeze', p.freeze > 0.5 ? 0 : 1)} />
        <LedToggle label="Flat" active={p.flatEnabled > 0.5} onClick={() => set('flatEnabled', p.flatEnabled > 0.5 ? 0 : 1)} />
        <LedToggle label="Cut" active={p.cutEnabled > 0.5} onClick={() => set('cutEnabled', p.cutEnabled > 0.5 ? 0 : 1)} />
      </div>

      <div className="group-label">Early Reflections</div>
      <div style={{ display: 'flex', gap: 10, alignItems: 'flex-start', flexWrap: 'wrap' }}>
        <LedToggle label="ER On" active={p.erEnabled > 0.5}
        onClick={() => set('erEnabled', p.erEnabled > 0.5 ? 0 : 1)} />
        <NumberField label="Amount" value={p.erAmount} onChange={(v) => set('erAmount', v)}
        display={fmt(ds(p.erAmount, 2, 55, 0.5), 1)} />
        <NumberField label="Rate" value={p.erRate} onChange={(v) => set('erRate', v)}
        display={fmt(ds(p.erRate, 0.07, 1.3, 0.5), 2, ' Hz')} />
        <NumberField label="Shape" value={p.erShape} onChange={(v) => set('erShape', v)}
        display={fmt(p.erShape, 2)} />
      </div>

      <div className="group-label">Chorus</div>
      <div style={{ display: 'flex', gap: 10, alignItems: 'flex-start', flexWrap: 'wrap' }}>
        <LedToggle label="Chorus" active={p.chorusEnabled > 0.5}
        onClick={() => set('chorusEnabled', p.chorusEnabled > 0.5 ? 0 : 1)} />
        <NumberField label="Amount" value={p.chorusAmount} onChange={(v) => set('chorusAmount', v)}
        display={fmt(dl(p.chorusAmount, 0.01, 4), 2)} />
        <NumberField label="Rate" value={p.chorusRate} onChange={(v) => set('chorusRate', v)}
        display={fmt(ds(p.chorusRate, 0.01, 8, 0.5), 2, ' Hz')} />
      </div>

      <div className="group-label">Input Filter</div>
      <div style={{ display: 'flex', gap: 10, alignItems: 'flex-start', flexWrap: 'wrap' }}>
        <LedToggle label="Lo Cut" active={p.loCutEnabled > 0.5}
        onClick={() => set('loCutEnabled', p.loCutEnabled > 0.5 ? 0 : 1)} />
        <NumberField label="Lo Freq" value={p.loCutFreq} onChange={(v) => set('loCutFreq', v)}
        display={fmt(ds(p.loCutFreq, 50, 18000, 0.3), 0, ' Hz')} />
        <LedToggle label="Hi Cut" active={p.hiCutEnabled > 0.5}
        onClick={() => set('hiCutEnabled', p.hiCutEnabled > 0.5 ? 0 : 1)} />
        <NumberField label="Hi Freq" value={p.hiCutFreq} onChange={(v) => set('hiCutFreq', v)}
        display={fmt(ds(p.hiCutFreq, 50, 18000, 0.3), 0, ' Hz')} />
      </div>
    </div>);

}

// ─── SATURATION panel ─────────────────────────────────────────────
function SaturationPanel({ p, set }) {
  return (
    <div className="panel scroll">
      <div className="panel-title">Saturation</div>

      <div className="group-label">Gain</div>
      <div className="knob-row">
        <Knob label="Gain" value={p.gainDb} onChange={(v) => set('gainDb', v)}
        display={fmt(dl(p.gainDb, -24, 12), 1, ' dB')} size="lg" />
        <Knob label="Trim" value={p.outputTrimDb} onChange={(v) => set('outputTrimDb', v)}
        display={fmt(dl(p.outputTrimDb, -12, 0), 1, ' dB')} size="md" />
      </div>

      <div className="group-label">Drive</div>
      <div className="knob-row">
        <Knob label="Drive" value={p.satDrive} onChange={(v) => set('satDrive', v)}
        display={fmt(p.satDrive * 100, 1, '%')} size="lg" />
        <Knob label="Mix" value={p.satMix} onChange={(v) => set('satMix', v)}
        display={fmt(dl(p.satMix, 0, 100), 0, '%')} size="md" />
        <Knob label="Tone" value={p.satTone} onChange={(v) => set('satTone', v)}
        display={fmt(p.satTone, 2)} size="md" />
      </div>

      {/* Envelope-style sliders to add visual interest like Aether ATK/DEC/SUS/REL */}
      <div className="group-label">Response</div>
      <div className="slider-row">
        <VSlider label="In" value={p.gainDb} onChange={(v) => set('gainDb', v)}
        display={fmt(dl(p.gainDb, -24, 12), 0, 'dB')} />
        <VSlider label="Drv" value={p.satDrive} onChange={(v) => set('satDrive', v)}
        display={fmt(p.satDrive * 100, 0, '%')} />
        <VSlider label="Mix" value={p.satMix} onChange={(v) => set('satMix', v)}
        display={fmt(dl(p.satMix, 0, 100), 0, '%')} />
        <VSlider label="Out" value={p.outputTrimDb} onChange={(v) => set('outputTrimDb', v)}
        display={fmt(dl(p.outputTrimDb, -12, 0), 0, 'dB')} />
      </div>
    </div>);

}

// ─── TIMBRE panel ─────────────────────────────────────────────────
function TimbrePanel({ p, set }) {
  return (
    <div className="panel scroll">
      <div className="panel-title">Timbre</div>

      <div className="group-label">Filter</div>
      <div className="knob-row" style={{ marginBottom: 8 }}>
        <Knob label="Cutoff" value={p.timbreCutoff} onChange={(v) => set('timbreCutoff', v)}
        display={fmt(ds(p.timbreCutoff, 20, 20000, 0.3), 0, ' Hz')} size="lg" />
        <Knob label="Resonance" value={p.timbreResonance} onChange={(v) => set('timbreResonance', v)}
        display={fmt(ds(p.timbreResonance, 0.5, 10, 0.5), 2)} size="md" />
      </div>
      <div className="knob-row">
        <Knob label="Pre-Drive" value={p.timbreDrivePre / 1} onChange={(v) => set('timbreDrivePre', v)}
        display={fmt(dl(p.timbreDrivePre, 0, 12), 1, ' dB')} size="md" />
      </div>

      {/* mini XY pad — a nice visual nod to the Aether reference */}
      <div className="group-label">XY · Cutoff × Reso</div>
      <XYPad
        x={p.timbreCutoff}
        y={1 - p.timbreResonance}
        onChange={(x, y) => {
          set('timbreCutoff', x);
          set('timbreResonance', 1 - y);
        }}
        xDisplay={fmt(ds(p.timbreCutoff, 20, 20000, 0.3), 0, ' Hz')}
        yDisplay={fmt(ds(p.timbreResonance, 0.5, 10, 0.5), 2)} />
      
    </div>);

}

function XYPad({ x, y, onChange, xDisplay, yDisplay }) {
  const ref = React.useRef(null);
  const onMouseDown = (e) => {
    e.preventDefault();
    const rect = ref.current.getBoundingClientRect();
    const update = (clientX, clientY) => {
      const nx = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
      const ny = Math.max(0, Math.min(1, (clientY - rect.top) / rect.height));
      onChange(nx, ny);
    };
    update(e.clientX, e.clientY);
    const onMove = (ev) => update(ev.clientX, ev.clientY);
    const onUp = () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  };

  return (
    <div
      ref={ref}
      onMouseDown={onMouseDown}
      style={{
        position: 'relative',
        width: '100%',
        aspectRatio: '1',
        maxWidth: 180,
        background: 'linear-gradient(135deg, #0a0810, #14111d)',
        border: '1px solid rgba(255,255,255,0.05)',
        borderRadius: 10,
        boxShadow: 'inset 0 2px 8px rgba(0,0,0,0.5)',
        cursor: 'crosshair',
        userSelect: 'none',
        marginTop: 6
      }}>
      
      {/* crosshair grid */}
      <div style={{
        position: 'absolute', left: 0, right: 0, top: '50%',
        height: 1, background: 'rgba(255,255,255,0.04)'
      }} />
      <div style={{
        position: 'absolute', top: 0, bottom: 0, left: '50%',
        width: 1, background: 'rgba(255,255,255,0.04)'
      }} />
      {/* corner ticks */}
      {['tl', 'tr', 'bl', 'br'].map((c) =>
      <span key={c} className={`cam-corner ${c}`} style={{
        width: 8, height: 8,
        top: c.includes('t') ? 4 : undefined,
        bottom: c.includes('b') ? 4 : undefined,
        left: c.includes('l') ? 4 : undefined,
        right: c.includes('r') ? 4 : undefined,
        borderColor: 'rgba(255,255,255,0.08)'
      }} />
      )}
      {/* point */}
      <div style={{
        position: 'absolute',
        left: `${x * 100}%`,
        top: `${y * 100}%`,
        width: 12, height: 12,
        borderRadius: '50%',
        background: 'var(--accent)',
        boxShadow: '0 0 12px var(--accent-glow), 0 0 24px var(--accent-dim)',
        transform: 'translate(-50%, -50%)',
        pointerEvents: 'none'
      }} />
      {/* readout */}
      <div style={{
        position: 'absolute', bottom: 6, left: 10, right: 10,
        display: 'flex', justifyContent: 'space-between',
        fontFamily: 'JetBrains Mono, monospace',
        fontSize: '0.5rem', letterSpacing: '0.16em',
        textTransform: 'uppercase',
        color: 'var(--accent)',
        pointerEvents: 'none'
      }}>
        <span>{xDisplay}</span>
        <span>{yDisplay}</span>
      </div>
    </div>);

}

// ─── SHIMMER panel ────────────────────────────────────────────────
function ShimmerPanel({ p, set }) {
  const pitchHz = Math.round(110 + 1650 * Math.pow(Math.max(0, p.pitchFrequency), 0.5));

  return (
    <div className="panel scroll">
      <div className="panel-title">Shimmer</div>

      <div className="group-label">Vibrato</div>
      <div className="knob-row">
        <Knob label="Rate" value={p.vibratoRate} onChange={(v) => set('vibratoRate', v)}
        display={fmt(ds(p.vibratoRate, 0.1, 8, 0.5), 2, ' Hz')} size="md" />
        <Knob label="Depth" value={p.vibratoDepth} onChange={(v) => set('vibratoDepth', v)}
        display={fmt(p.vibratoDepth, 2)} size="md" />
      </div>
      <div className="num-row" style={{ marginTop: 6 }}>
        <NumberField label="Fade In" value={p.vibratoFadeIn} onChange={(v) => set('vibratoFadeIn', v)}
        display={fmt(ds(p.vibratoFadeIn, 0, 500, 0.5), 0, ' ms')} />
      </div>

      <div className="group-label">Pitch Shifter</div>
      <div className="knob-row">
        <Knob label="Frequency" value={p.pitchFrequency} onChange={(v) => set('pitchFrequency', v)}
        display={`${pitchHz} Hz`} size="lg" />
      </div>
      <div style={{ marginTop: 8 }}>
        <div className="numf-label">Oct Step</div>
        <div style={{ display: 'flex', gap: 4, marginTop: 4 }}>
          <LedToggle label="Cont." active={p.pitchOctaveStep < 0.5} onClick={() => set('pitchOctaveStep', 0)} />
          <LedToggle label="Snap" active={p.pitchOctaveStep >= 0.5} onClick={() => set('pitchOctaveStep', 1)} />
        </div>
      </div>
    </div>);

}

Object.assign(window, { ReverbPanel, SaturationPanel, TimbrePanel, ShimmerPanel });