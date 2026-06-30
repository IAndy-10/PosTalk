<script lang="ts">
    import { params, setParameterValue } from './state/store';
    import { bridge } from './bridge/bridge';
    import type { ParameterId } from './types/parameters';

    import Knob        from './components/Knob.svelte';
    import VSlider     from './components/VSlider.svelte';
    import LedToggle   from './components/LedToggle.svelte';
    import NumberPill  from './components/NumberPill.svelte';
    import XYPad       from './components/XYPad.svelte';
    import GestureMap  from './components/GestureMap.svelte';
    import Lamp        from './components/Lamp.svelte';
    import Camera            from './components/Camera.svelte';
    import GestureController from './components/GestureController.svelte';
    import Tooltip           from './components/Tooltip.svelte';

    const {
        gainDb, outputTrimDb,
        satDrive, satMix, satTone,
        timbreCutoff, timbreResonance, timbreDrivePre,
        vibratoRate, vibratoDepth, vibratoFadeIn,
        pitchFrequency, pitchOctaveStep,
        sustainEnabled,
        reverbMode, crossoverFreq, diffusion, scale, decay, damping, feedback,
        highFilterType, chorusEnabled, chorusAmount, chorusRate,
        reflectGain, diffuseGain, dryWet,
        predelay, smooth, size, freeze, flatEnabled, stereo, density,
        loCutEnabled, hiCutEnabled, loCutFreq, hiCutFreq,
        erEnabled, erAmount, erRate, erShape,
    } = params;

    // ── Header state (visual only) ─────────────────────────────────
    let bypassed  = false;
    let pwr       = true;
    let mono      = false;

    // ── Gesture simulation from hover ──────────────────────────────
    let hoveredGesture: string | null = null;

    // ── Presets ────────────────────────────────────────────────────
    let activePreset: number | null = null;

    const PRESET_NAMES = ['Abyss', 'Chamber', 'Room', 'Studio', 'Hall', 'Infinity', 'IPA'];
    const PRESETS: Record<string, number>[] = [
      { decay: 0.22,  damping: 0.90, crossoverFreq: 0.16, size: 0.20, diffusion: 0.30, feedback: 0.72, dryWet: 0.60, reverbMode: 1, scale: 0.25, density: 0,    smooth: 0, highFilterType: 0, satDrive: 0.30, satMix: 0.35, satTone: 0.25, timbreCutoff: 0.38, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.10, stereo: 0.33, erEnabled: 0, erAmount: 0.05, erShape: 0.3, erRate: 0.40, chorusEnabled: 0, chorusAmount: 0, chorusRate: 0.43, reflectGain: 0.50, diffuseGain: 0.67, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75, vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45, pitchFrequency: 0.04, pitchOctaveStep: 0, sustainEnabled: 0 },
      { decay: 0.32,  damping: 0.72, crossoverFreq: 0.28, size: 0.35, diffusion: 0.50, feedback: 0.75, dryWet: 0.50, reverbMode: 1, scale: 0.35, density: 0.33, smooth: 0, highFilterType: 0, satDrive: 0.20, satMix: 0.20, satTone: 0.40, timbreCutoff: 0.54, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.155, stereo: 0.46, erEnabled: 1, erAmount: 0.55, erShape: 0.3, erRate: 0.43, chorusEnabled: 0, chorusAmount: 0, chorusRate: 0.43, reflectGain: 0.61, diffuseGain: 0.72, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75, vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45, pitchFrequency: 0.04, pitchOctaveStep: 0, sustainEnabled: 0 },
      { decay: 0.35,  damping: 0.55, crossoverFreq: 0.48, size: 0.48, diffusion: 0.60, feedback: 0.78, dryWet: 0.45, reverbMode: 1, scale: 0.50, density: 0.67, smooth: 0, highFilterType: 0, satDrive: 0.10, satMix: 0.10, satTone: 0.60, timbreCutoff: 0.66, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.20,  stereo: 0.54, erEnabled: 1, erAmount: 0.66, erShape: 0.5, erRate: 0.59, chorusEnabled: 0, chorusAmount: 0, chorusRate: 0.43, reflectGain: 0.69, diffuseGain: 0.78, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75, vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45, pitchFrequency: 0.04, pitchOctaveStep: 0, sustainEnabled: 0 },
      { decay: 0.40,  damping: 0.38, crossoverFreq: 0.70, size: 0.55, diffusion: 0.68, feedback: 0.82, dryWet: 0.40, reverbMode: 0, scale: 0.55, density: 1,    smooth: 0, highFilterType: 0, satDrive: 0,    satMix: 0,    satTone: 0.80, timbreCutoff: 0.76, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.237, stereo: 0.63, erEnabled: 1, erAmount: 0.79, erShape: 0.5, erRate: 0.59, chorusEnabled: 1, chorusAmount: 0.15, chorusRate: 0.43, reflectGain: 0.75, diffuseGain: 0.83, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75, vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45, pitchFrequency: 0.04, pitchOctaveStep: 0, sustainEnabled: 0 },
      { decay: 0.50,  damping: 0.20, crossoverFreq: 0.86, size: 0.72, diffusion: 0.75, feedback: 0.85, dryWet: 0.35, reverbMode: 0, scale: 0.70, density: 1,    smooth: 0, highFilterType: 0, satDrive: 0,    satMix: 0,    satTone: 1.0,  timbreCutoff: 0.86, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.265, stereo: 0.75, erEnabled: 1, erAmount: 0.90, erShape: 0.6, erRate: 0.59, chorusEnabled: 1, chorusAmount: 0.20, chorusRate: 0.39, reflectGain: 0.78, diffuseGain: 0.89, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75, vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45, pitchFrequency: 0.04, pitchOctaveStep: 0, sustainEnabled: 0 },
      { decay: 0.77,  damping: 0.08, crossoverFreq: 0.97, size: 0.90, diffusion: 0.85, feedback: 0.90, dryWet: 0.28, reverbMode: 0, scale: 0.90, density: 1,    smooth: 0, highFilterType: 0, satDrive: 0,    satMix: 0,    satTone: 1.0,  timbreCutoff: 0.94, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.316, stereo: 0.92, erEnabled: 1, erAmount: 0.98, erShape: 0.75, erRate: 0.59, chorusEnabled: 1, chorusAmount: 0.25, chorusRate: 0.31, reflectGain: 0.83, diffuseGain: 0.94, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75, vibratoRate: 0.42, vibratoDepth: 0.15, vibratoFadeIn: 0.45, pitchFrequency: 0.218, pitchOctaveStep: 1, sustainEnabled: 0 },
      { decay: 0.77,  damping: 0.08, crossoverFreq: 0.97, size: 0.90, diffusion: 0.85, feedback: 0.90, dryWet: 0.98, reverbMode: 0, scale: 0.90, density: 1,    smooth: 0, highFilterType: 0, satDrive: 0,    satMix: 0,    satTone: 1.0,  timbreCutoff: 0.94, timbreResonance: 0.15, timbreDrivePre: 0, predelay: 0.316, stereo: 0.92, erEnabled: 1, erAmount: 0.98, erShape: 0.75, erRate: 0.59, chorusEnabled: 1, chorusAmount: 0.25, chorusRate: 0.31, reflectGain: 0.83, diffuseGain: 0.94, gainDb: 0.667, outputTrimDb: 1, freeze: 0, flatEnabled: 0, cutEnabled: 0, loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 1, hiCutFreq: 0.35, vibratoRate: 1.42, vibratoDepth: 0.15, vibratoFadeIn: 0.45, pitchFrequency: 0.04, pitchOctaveStep: 0, sustainEnabled: 0 },
    ];

    function applyPreset(idx: number) {
        activePreset = idx;
        for (const [k, v] of Object.entries(PRESETS[idx])) send(k as ParameterId, v);
    }

    // ── DSP bridge helpers ─────────────────────────────────────────
    function send(id: ParameterId, v: number) {
        if (!isFinite(v) || isNaN(v)) return;
        setParameterValue(id, v);
        bridge.sendParameterChange(id, v);
    }

    // Momentary CUT — rising edge then auto-reset
    function handleCutNow() {
        send('cutNow', 1);
        setTimeout(() => send('cutNow', 0), 100);
    }

    // Reset pitch shifter to 440 Hz (unity — no pitch change)
    function handlePitchReset() {
        send('pitchFrequency', 0.04);
    }

    // normalized ↔ display helpers
    const dl = (n: number, mn: number, mx: number) => mn + (mx - mn) * n;
    const ds = (n: number, mn: number, mx: number, sk: number) =>
        mn + (mx - mn) * Math.pow(Math.max(0, n), 1 / sk);
</script>

<main class="scene">
  <div class="plugin" class:bypassed>

    <!-- ── Header ── -->
    <div class="header">
      <div class="brand">
        <span class="brand-name">POSTALK</span>
        <span class="brand-sep">—</span>
        <span class="brand-sub">DSP · 01 · REVERB</span>
      </div>
      <div class="header-controls">
        <Lamp label="PWR"    on={pwr}      on:click={() => pwr      = !pwr}      />
        <Lamp label="Bypass" on={bypassed} on:click={() => bypassed = !bypassed} />
        <Lamp label="Mono"   on={mono}     on:click={() => mono     = !mono}     />
      </div>
    </div>

    <div class="body">

      <!-- ══ TOP ROW  1fr / 3fr / 1fr ══ -->
      <div class="row-top">

        <!-- Presets -->
        <div class="panel">
          <div class="panel-title">Presets</div>
          <div class="presets-grid">
            {#each PRESET_NAMES as name, i}
              <button
                class="preset-btn"
                class:active={activePreset === i}
                title={name}
                on:click={() => applyPreset(i)}
              >{i + 1}</button>
            {/each}
          </div>
          <div class="preset-name">
            {activePreset !== null ? PRESET_NAMES[activePreset] : '— select —'}
          </div>
        </div>

        <!-- Camera card -->
        <div class="camera-card">
          <span class="cam-corner tl" />
          <span class="cam-corner tr" />
          <span class="cam-corner bl" />
          <span class="cam-corner br" />
          <div class="cam-status">
            <span class="dot" />
            hand-tracking · ready
          </div>
          <Camera />
          <GestureController />
          <div class="cam-readout">
            <div class="cam-readout-group">
              <span>SUSTAIN <b>{$sustainEnabled > 0.5 ? 'ON' : 'OFF'}</b></span>
            </div>
            <div class="cam-readout-group">
              <span>GESTURE <b>{hoveredGesture ?? 'none'}</b></span>
            </div>
          </div>
        </div>

        <!-- Gesture map -->
        <div class="panel">
          <div class="panel-title">Gestures</div>
          <GestureMap
            activeGesture={hoveredGesture}
            on:simulate={e => { hoveredGesture = e.detail; }}
          />
        </div>

      </div>

      <!-- ══ BOTTOM ROW  1.6fr / 1fr / 1fr / 1fr ══ -->
      <div class="row-bottom">

        <!-- ─ REVERB ─ -->
        <div class="panel scroll">
          <div class="panel-title">Reverb</div>

          <div class="group-label">Performance</div>
          <div class="perf-row">
            <LedToggle label="Sustain" active={$sustainEnabled > 0.5}
              on:change={e => send('sustainEnabled', e.detail ? 1 : 0)} />
            <button class="cut-btn" on:click={handleCutNow}>CUT</button>
          </div>

          <div class="group-label">Core</div>
          <div class="knob-row">
            <Knob label="Decay"
              value={$decay}
              display="{ds($decay, 200, 60000, 0.3).toFixed(0)} ms"
              on:change={e => send('decay', e.detail)} />
            <Knob label="Diffusion"
              value={$diffusion}
              display={$diffusion.toFixed(2)}
              on:change={e => send('diffusion', e.detail)} />
            <Knob label="Size"
              value={$size}
              display={$size.toFixed(2)}
              on:change={e => send('size', e.detail)} />
            <Knob label="Damping"
              value={$damping}
              display={$damping.toFixed(2)}
              on:change={e => send('damping', e.detail)} />
            <Knob label="Feedback"
              value={$feedback}
              display={$feedback.toFixed(2)}
              on:change={e => send('feedback', e.detail)} />
            <Knob label="Stereo"
              value={$stereo}
              display="{dl($stereo, 0, 120).toFixed(0)}°"
              on:change={e => send('stereo', e.detail)} />
          </div>

          <div class="group-label">Mix</div>
          <div class="knob-row">
            <Knob label="Dry/Wet"
              value={$dryWet}
              display="{dl($dryWet, 0, 100).toFixed(0)}%"
              on:change={e => send('dryWet', e.detail)} />
            <NumberPill label="Predelay"
              value={$predelay}
              display="{ds($predelay, 0, 500, 0.5).toFixed(1)} ms"
              on:change={e => send('predelay', e.detail)} />
          </div>
        </div>

        <!-- ─ SATURATION ─ -->
        <div class="panel scroll">
          <div class="panel-title">Saturation</div>

          <div class="group-label">Response</div>
          <div class="slider-row">
            <VSlider label="In"
              value={$gainDb}
              display="{dl($gainDb, -24, 12).toFixed(1)} dB"
              on:change={e => send('gainDb', e.detail)} />
            <VSlider label="Drive"
              value={$satDrive}
              display="{($satDrive * 100).toFixed(0)}%"
              on:change={e => send('satDrive', e.detail)} />
            <VSlider label="Mix"
              value={$satMix}
              display="{dl($satMix, 0, 100).toFixed(0)}%"
              on:change={e => send('satMix', e.detail)} />
            <VSlider label="Out"
              value={$outputTrimDb}
              display="{dl($outputTrimDb, -12, 0).toFixed(1)} dB"
              on:change={e => send('outputTrimDb', e.detail)} />
          </div>

          <div class="group-label">Tone</div>
          <div class="knob-row">
            <Knob label="Tone" size="lg"
              value={$satTone}
              display={$satTone.toFixed(2)}
              on:change={e => send('satTone', e.detail)} />
          </div>
        </div>

        <!-- ─ TIMBRE ─ -->
        <div class="panel scroll">
          <div class="panel-title">Timbre</div>

          <div class="group-label">Cutoff × Resonance</div>
          <XYPad
            x={$timbreCutoff}
            y={1 - $timbreResonance}
            xDisplay="{ds($timbreCutoff, 20, 20000, 0.3).toFixed(0)} Hz"
            yDisplay={ds($timbreResonance, 0.5, 10, 0.5).toFixed(2)}
            on:change={e => {
              send('timbreCutoff',     e.detail.x);
              send('timbreResonance',  1 - e.detail.y);
            }}
          />

          <div class="group-label">Pre-Drive</div>
          <div class="knob-row">
            <Knob label="Pre-Drive"
              value={$timbreDrivePre}
              display="{dl($timbreDrivePre, 0, 12).toFixed(1)} dB"
              on:change={e => send('timbreDrivePre', e.detail)} />
          </div>
        </div>

        <!-- ─ SHIMMER ─ -->
        <div class="panel scroll">
          <div class="panel-title">Shimmer</div>

          <div class="group-label">Vibrato</div>
          <div class="knob-row">
            <Knob label="Rate"
              value={$vibratoRate}
              display="{ds($vibratoRate, 0.1, 8, 0.5).toFixed(2)} Hz"
              on:change={e => send('vibratoRate', e.detail)} />
            <Knob label="Depth"
              value={$vibratoDepth}
              display={$vibratoDepth.toFixed(2)}
              on:change={e => send('vibratoDepth', e.detail)} />
          </div>
          <div class="num-row" style="margin-top: 6px;">
            <NumberPill label="Fade In"
              value={$vibratoFadeIn}
              display="{ds($vibratoFadeIn, 0, 500, 0.5).toFixed(0)} ms"
              on:change={e => send('vibratoFadeIn', e.detail)} />
          </div>

          <div class="group-label">Pitch Shifter</div>
          <div class="perf-row">
            <button class="cut-btn" on:click={handlePitchReset}>440</button>
          </div>
          <div class="knob-row">
            <Knob label="Frequency" size="lg"
              value={$pitchFrequency}
              display="{Math.round(110 + 1650 * Math.pow(Math.max(0, $pitchFrequency), 0.5))} Hz"
              on:change={e => send('pitchFrequency', e.detail)} />
          </div>
          <div class="oct-step-wrap">
            <div class="oct-step-label">Oct Step</div>
            <div class="oct-step-row">
              <LedToggle label="Cont." active={$pitchOctaveStep < 0.5}
                on:change={() => send('pitchOctaveStep', 0)} />
              <LedToggle label="Snap"  active={$pitchOctaveStep >= 0.5}
                on:change={() => send('pitchOctaveStep', 1)} />
            </div>
          </div>
        </div>

      </div>

      <!-- ── Footer ── -->
      <div class="footer">
        <span>postalk · spectral processor · v0.1</span>
        <div class="footer-right">
          <span>{bypassed ? 'bypassed' : pwr ? 'active' : 'standby'}</span>
          <span class="pulse-dot" />
        </div>
      </div>

    </div>
  </div>
</main>

<Tooltip />

<style>
  /* ── CSS custom properties ── */
  :global(:root) {
    --accent:      #ffb648;
    --accent-2:    #ffb648;
    --accent-dim:  rgba(255, 182, 72, 0.42);
    --accent-soft: rgba(255, 182, 72, 0.12);
    --accent-glow: rgba(255, 182, 72, 0.55);
    --accent-line: rgba(255, 182, 72, 0.20);

    --bg-0: #0a0b12;

    --glass-bg:        rgba(255, 255, 255, 0.022);
    --glass-bg-hi:     rgba(255, 255, 255, 0.035);
    --glass-border:    rgba(255, 255, 255, 0.055);
    --glass-border-hi: rgba(255, 255, 255, 0.09);
    --glass-blur:      blur(24px) saturate(125%);

    --text-hi:  rgba(232, 226, 248, 0.92);
    --text:     rgba(204, 198, 224, 0.70);
    --text-lo:  rgba(170, 165, 195, 0.42);
    --text-xlo: rgba(170, 165, 195, 0.24);

    --gap:     18px;
    --pad:     22px;
    --row-h:   320px;
    --knob-lg: 76px;
    --knob-md: 60px;
    --knob-sm: 46px;
  }

  :global(body) {
    margin: 0; padding: 0;
    font-family: 'Inter', system-ui, sans-serif;
    font-weight: 300;
    background:
      radial-gradient(ellipse 60% 50% at 20% 0%,   rgba(110, 90, 150, 0.16), transparent 70%),
      radial-gradient(ellipse 60% 60% at 85% 100%, rgba(255, 130, 50, 0.10), transparent 65%),
      radial-gradient(ellipse 80% 60% at 50% 60%,  rgba(50, 40, 80, 0.30),   transparent 70%),
      #0a0b12;
    background-attachment: fixed;
    color: rgba(204, 198, 224, 0.70);
    -webkit-font-smoothing: antialiased;
    min-height: 100vh;
    overflow-y: auto;
    user-select: none;
    cursor: default;
  }

  /* ── Scene + plugin shell ── */
  .scene {
    min-height: 100vh;
    padding: 1rem;
  }

  .plugin {
    width: 100%; max-width: 1480px;
    margin: 0 auto;
    padding: 28px 32px 36px;
    position: relative;
  }

  .plugin.bypassed .body {
    opacity: 0.30; pointer-events: none; filter: grayscale(0.4);
    transition: opacity 0.2s, filter 0.2s;
  }

  /* ── Header ── */
  .header {
    display: flex; align-items: center; justify-content: space-between;
    padding: 4px 6px 22px;
  }
  .brand { display: flex; align-items: baseline; gap: 16px; }
  .brand-name {
    font-family: 'Cormorant Garamond', 'Times New Roman', serif;
    font-size: 2rem; font-weight: 400; letter-spacing: 0.22em;
    text-transform: uppercase;
    color: var(--text-hi);
    text-shadow: 0 0 28px rgba(220, 200, 255, 0.16);
  }
  .brand-sep {
    color: var(--text-xlo); font-weight: 200;
    font-size: 1rem; transform: translateY(-2px);
  }
  .brand-sub {
    font-family: 'JetBrains Mono', 'Menlo', monospace;
    font-size: 0.62rem; font-weight: 400; letter-spacing: 0.38em;
    color: var(--text-lo); text-transform: uppercase;
  }
  .header-controls { display: flex; gap: 26px; align-items: flex-end; }

  /* ── Layout ── */
  .body { display: flex; flex-direction: column; gap: var(--gap); }

  .row-top {
    display: grid;
    grid-template-columns: 1fr 3fr 1fr;
    gap: var(--gap);
    height: var(--row-h);
  }

  .row-bottom {
    display: grid;
    grid-template-columns: 1.6fr 1fr 1fr 1fr;
    gap: var(--gap);
    align-items: stretch;
  }

  /* ── Glass panel ── */
  .panel {
    background: var(--glass-bg);
    border: 1px solid var(--glass-border);
    border-radius: 18px;
    padding: var(--pad);
    position: relative;
    backdrop-filter: var(--glass-blur);
    -webkit-backdrop-filter: var(--glass-blur);
    box-shadow:
      inset 0 1px 0 rgba(255,255,255,0.04),
      0 1px 0 rgba(255,255,255,0.025);
    overflow: hidden;
    transition: border-color 0.25s, background 0.25s;
  }
  /* inner highlight */
  .panel::after {
    content: ''; position: absolute; inset: 0;
    border-radius: inherit; pointer-events: none;
    background: radial-gradient(ellipse 80% 60% at 50% 0%, rgba(255,255,255,0.03), transparent 60%);
  }
  .panel.scroll {
    max-height: calc(100vh - 80px);
    overflow-y: auto;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.08) transparent;
  }
  .panel.scroll::-webkit-scrollbar { width: 4px; }
  .panel.scroll::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.06); border-radius: 3px; }

  .panel-title {
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.62rem; font-weight: 400; letter-spacing: 0.34em;
    text-transform: uppercase; color: var(--text);
    margin-bottom: 18px;
    display: flex; align-items: center; gap: 10px;
  }
  .panel-title::after {
    content: ''; flex: 1; height: 1px;
    background: linear-gradient(to right, var(--glass-border-hi), transparent);
  }

  .group-label {
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.54rem; font-weight: 400; letter-spacing: 0.30em;
    text-transform: uppercase; color: var(--text-lo);
    margin: 22px 0 12px; padding-left: 2px;
  }
  .group-label:first-of-type { margin-top: 4px; }

  /* ── Presets ── */
  .presets-grid {
    display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px;
    margin-top: 6px;
  }
  .preset-btn {
    aspect-ratio: 1; border-radius: 50%;
    background: var(--glass-bg-hi);
    border: 1px solid var(--glass-border);
    backdrop-filter: blur(8px); -webkit-backdrop-filter: blur(8px);
    color: var(--text-lo);
    font-family: 'JetBrains Mono', monospace; font-size: 0.72rem;
    cursor: pointer; transition: all 0.15s;
    display: flex; align-items: center; justify-content: center;
  }
  .preset-btn:hover { color: var(--accent); border-color: var(--accent-line); }
  .preset-btn.active {
    color: var(--accent); border-color: var(--accent-line);
    background: rgba(255,182,72,0.08);
    box-shadow: 0 0 18px var(--accent-soft);
  }
  .preset-name {
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.56rem; font-weight: 400; letter-spacing: 0.26em;
    text-transform: uppercase; color: var(--accent);
    text-align: center; margin-top: 16px; min-height: 0.7rem;
  }

  /* ── Camera card ── */
  .camera-card {
    position: relative;
    border: 1px solid var(--glass-border);
    border-radius: 18px;
    overflow: hidden;
    background: rgba(8, 8, 14, 0.45);
    backdrop-filter: var(--glass-blur);
    -webkit-backdrop-filter: var(--glass-blur);
  }
  .cam-corner {
    position: absolute; width: 14px; height: 14px;
    border: 1px solid var(--accent-line);
    z-index: 4; pointer-events: none;
  }
  .cam-corner.tl { top: 10px; left: 10px; border-right: none; border-bottom: none; }
  .cam-corner.tr { top: 10px; right: 10px; border-left: none; border-bottom: none; }
  .cam-corner.bl { bottom: 10px; left: 10px; border-right: none; border-top: none; }
  .cam-corner.br { bottom: 10px; right: 10px; border-left: none; border-top: none; }

  .cam-status {
    position: absolute; top: 18px; left: 24px; z-index: 5;
    display: flex; align-items: center; gap: 8px;
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.54rem; letter-spacing: 0.26em; text-transform: uppercase;
    color: var(--text-lo);
    background: rgba(0,0,0,0.30); padding: 5px 12px; border-radius: 12px;
    backdrop-filter: blur(8px); border: 1px solid var(--glass-border);
  }
  .cam-status .dot {
    width: 6px; height: 6px; border-radius: 50%;
    background: var(--accent); box-shadow: 0 0 6px var(--accent-glow);
    animation: pulse 1.6s ease-in-out infinite;
  }

  .cam-readout {
    position: absolute; bottom: 18px; left: 24px; right: 24px; z-index: 5;
    display: flex; justify-content: space-between; align-items: center;
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.52rem; letter-spacing: 0.20em; text-transform: uppercase;
    color: var(--text-lo);
  }
  .cam-readout-group { display: flex; gap: 18px; }

  /* ── Performance row ── */
  .perf-row { display: flex; align-items: center; gap: 12px; margin-bottom: 4px; }
  .cut-btn {
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.58rem; font-weight: 400; letter-spacing: 0.26em;
    text-transform: uppercase;
    padding: 8px 20px;
    background: rgba(255, 80, 60, 0.08); border: 1px solid rgba(255, 80, 60, 0.32);
    color: rgba(255, 140, 120, 0.9); border-radius: 9px;
    cursor: pointer; transition: all 0.15s; backdrop-filter: blur(8px);
  }
  .cut-btn:hover  { background: rgba(255, 80, 60, 0.16); }
  .cut-btn:active { background: rgba(255, 80, 60, 0.24); }

  /* ── Param grids ── */
  .knob-row   { display: flex; flex-wrap: wrap; gap: 18px 22px; align-items: flex-start; }
  .num-row    { display: flex; flex-wrap: wrap; gap: 10px 14px; align-items: flex-start; }
  .slider-row { display: flex; gap: 8px; align-items: flex-end; justify-content: space-around; flex-wrap: wrap; padding: 8px 2px 0; }

  /* Oct Step */
  .oct-step-wrap { margin-top: 12px; }
  .oct-step-label {
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.52rem; letter-spacing: 0.26em;
    text-transform: uppercase; color: var(--text-lo);
    margin-bottom: 6px;
  }
  .oct-step-row { display: flex; gap: 4px; }

  /* cam readout value highlight */
  :global(.cam-readout b) { color: var(--accent); font-weight: 400; margin-left: 6px; }

  /* ── Footer ── */
  .footer {
    display: flex; justify-content: space-between; align-items: center;
    padding: 18px 6px 0; margin-top: 10px;
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.55rem; font-weight: 400; letter-spacing: 0.30em;
    text-transform: uppercase; color: var(--text-xlo);
  }
  .footer-right { display: flex; align-items: center; gap: 10px; }
  .pulse-dot {
    display: inline-block; width: 6px; height: 6px; border-radius: 50%;
    background: var(--accent); box-shadow: 0 0 8px var(--accent-glow);
    animation: pulse 1.6s ease-in-out infinite;
  }

  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50%       { opacity: 0.35; }
  }
</style>
