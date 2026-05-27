<script lang="ts">
    import { params, setParameterValue } from './state/store';
    import { bridge } from './bridge/bridge';
    import type { ParameterId } from './types/parameters';

    import Number   from './components/Number.svelte';
    import Toggle   from './components/Toggle.svelte';
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
        loCutEnabled, hiCutEnabled, loCutFreq, hiCutFreq,
        erEnabled, erAmount, erRate, erShape,
        reverbMode, crossoverFreq, diffusion, scale, decay, damping, feedback,
        highFilterType, chorusEnabled, chorusAmount, chorusRate,
        reflectGain, diffuseGain, dryWet,
        predelay, smooth, size, freeze, flatEnabled, cutEnabled, stereo, density,
    } = params;

    let bypassed    = false;
    let activePreset: number | null = null;

    // ── Presets: journey from dark (1) to bright (6) ──────────────────────────
    // All values are normalized APVTS 0-1 (same units the store and bridge expect).
    // Conversion reference (matches App.svelte Number bindings):
    //   decay       200–60000 ms  skew 0.3
    //   crossoverFreq 200–8000 Hz skew 0.5
    //   timbreCutoff  20–20000 Hz skew 0.3
    //   predelay    0–500 ms      skew 0.5
    //   erAmount    2–55          skew 0.5
    //   chorusRate  0.01–8 Hz     skew 0.5
    //   others: linear or raw 0-1
    const PRESET_NAMES = ['Abyss', 'Chamber', 'Room', 'Studio', 'Hall', 'Infinity', 'IPA'];
    const PRESETS: Record<string, number>[] = [
      // ── 1 · Abyss ── ultra-dark, compressed, cave-like
      {
        decay: 0.22,  damping: 0.90, crossoverFreq: 0.16, size: 0.20,
        diffusion: 0.30, feedback: 0.72, dryWet: 0.60, reverbMode: 1,
        scale: 0.25,  density: 0,   smooth: 0, highFilterType: 0,
        satDrive: 0.30, satMix: 0.35, satTone: 0.25,
        timbreCutoff: 0.38, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.10, stereo: 0.33,
        erEnabled: 0,  erAmount: 0.05, erShape: 0.3,  erRate: 0.40,
        chorusEnabled: 0, chorusAmount: 0, chorusRate: 0.43,
        reflectGain: 0.50, diffuseGain: 0.67,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75,
        vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45,
        pitchFrequency: 0.04, pitchOctaveStep: 0,
        sustainEnabled: 0,
      },
      // ── 2 · Chamber ── dark, intimate, warm
      {
        decay: 0.32,  damping: 0.72, crossoverFreq: 0.28, size: 0.35,
        diffusion: 0.50, feedback: 0.75, dryWet: 0.50, reverbMode: 1,
        scale: 0.35,  density: 0.33, smooth: 0, highFilterType: 0,
        satDrive: 0.20, satMix: 0.20, satTone: 0.40,
        timbreCutoff: 0.54, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.155, stereo: 0.46,
        erEnabled: 1,  erAmount: 0.55, erShape: 0.3,  erRate: 0.43,
        chorusEnabled: 0, chorusAmount: 0, chorusRate: 0.43,
        reflectGain: 0.61, diffuseGain: 0.72,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75,
        vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45,
        pitchFrequency: 0.04, pitchOctaveStep: 0,
        sustainEnabled: 0,
      },
      // ── 3 · Room ── neutral, balanced, natural
      {
        decay: 0.35,  damping: 0.55, crossoverFreq: 0.48, size: 0.48,
        diffusion: 0.60, feedback: 0.78, dryWet: 0.45, reverbMode: 1,
        scale: 0.50,  density: 0.67, smooth: 0, highFilterType: 0,
        satDrive: 0.10, satMix: 0.10, satTone: 0.60,
        timbreCutoff: 0.66, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.20, stereo: 0.54,
        erEnabled: 1,  erAmount: 0.66, erShape: 0.5,  erRate: 0.59,
        chorusEnabled: 0, chorusAmount: 0, chorusRate: 0.43,
        reflectGain: 0.69, diffuseGain: 0.78,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75,
        vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45,
        pitchFrequency: 0.04, pitchOctaveStep: 0,
        sustainEnabled: 0,
      },
      // ── 4 · Studio ── neutral-bright, clean, open
      {
        decay: 0.40,  damping: 0.38, crossoverFreq: 0.70, size: 0.55,
        diffusion: 0.68, feedback: 0.82, dryWet: 0.40, reverbMode: 0,
        scale: 0.55,  density: 1,   smooth: 0, highFilterType: 0,
        satDrive: 0,  satMix: 0,    satTone: 0.80,
        timbreCutoff: 0.76, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.237, stereo: 0.63,
        erEnabled: 1,  erAmount: 0.79, erShape: 0.5,  erRate: 0.59,
        chorusEnabled: 1, chorusAmount: 0.15, chorusRate: 0.43,
        reflectGain: 0.75, diffuseGain: 0.83,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75,
        vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45,
        pitchFrequency: 0.04, pitchOctaveStep: 0,
        sustainEnabled: 0,
      },
      // ── 5 · Hall ── bright, large, spatial
      {
        decay: 0.50,  damping: 0.20, crossoverFreq: 0.86, size: 0.72,
        diffusion: 0.75, feedback: 0.85, dryWet: 0.35, reverbMode: 0,
        scale: 0.70,  density: 1,   smooth: 0, highFilterType: 0,
        satDrive: 0,  satMix: 0,    satTone: 1.0,
        timbreCutoff: 0.86, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.265, stereo: 0.75,
        erEnabled: 1,  erAmount: 0.90, erShape: 0.6,  erRate: 0.59,
        chorusEnabled: 1, chorusAmount: 0.20, chorusRate: 0.39,
        reflectGain: 0.78, diffuseGain: 0.89,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75,
        vibratoRate: 0.49, vibratoDepth: 0, vibratoFadeIn: 0.45,
        pitchFrequency: 0.04, pitchOctaveStep: 0,
        sustainEnabled: 0,
      },
      // ── 6 · Infinity ── vast, shimmering, borderless
      {
        decay: 0.77,  damping: 0.08, crossoverFreq: 0.97, size: 0.90,
        diffusion: 0.85, feedback: 0.90, dryWet: 0.28, reverbMode: 0,
        scale: 0.90,  density: 1,   smooth: 0, highFilterType: 0,
        satDrive: 0,  satMix: 0,    satTone: 1.0,
        timbreCutoff: 0.94, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.316, stereo: 0.92,
        erEnabled: 1,  erAmount: 0.98, erShape: 0.75, erRate: 0.59,
        chorusEnabled: 1, chorusAmount: 0.25, chorusRate: 0.31,
        reflectGain: 0.83, diffuseGain: 0.94,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 0, hiCutFreq: 0.75,
        vibratoRate: 0.42, vibratoDepth: 0.15, vibratoFadeIn: 0.45,
        pitchFrequency: 0.218, pitchOctaveStep: 1,  // +1 octave snap (880 Hz) for shimmer
        sustainEnabled: 0,
      },
      // ── 7 · IPA 
      {
        decay: 0.77,  damping: 0.08, crossoverFreq: 0.97, size: 0.90,
        diffusion: 0.85, feedback: 0.90, dryWet: 0.98, reverbMode: 0,
        scale: 0.90,  density: 1,   smooth: 0, highFilterType: 0,
        satDrive: 0,  satMix: 0,    satTone: 1.0,
        timbreCutoff: 0.94, timbreResonance: 0.15, timbreDrivePre: 0,
        predelay: 0.316, stereo: 0.92,
        erEnabled: 1,  erAmount: 0.98, erShape: 0.75, erRate: 0.59,
        chorusEnabled: 1, chorusAmount: 0.25, chorusRate: 0.31,
        reflectGain: 0.83, diffuseGain: 0.94,
        gainDb: 0.667, outputTrimDb: 1,
        freeze: 0, flatEnabled: 0, cutEnabled: 0,
        loCutEnabled: 0, loCutFreq: 0.20, hiCutEnabled: 1, hiCutFreq: 0.35,
        vibratoRate: 1.42, vibratoDepth: 0.15, vibratoFadeIn: 0.45,
        pitchFrequency: 0.04, pitchOctaveStep: 0, 
        sustainEnabled: 0,
      },
    ];

    function applyPreset(idx: number) {
      activePreset = idx;
      const p = PRESETS[idx];
      for (const [k, v] of Object.entries(p)) {
        send(k as ParameterId, v);
      }
    }

    function send(id: ParameterId, v: number) {
        if (!isFinite(v) || isNaN(v)) return;
        setParameterValue(id, v);
        bridge.sendParameterChange(id, v);
    }

    // display value → normalized 0-1
    const nl = (v: number, mn: number, mx: number) =>
        Math.min(1, Math.max(0, (v - mn) / (mx - mn)));
    const ns = (v: number, mn: number, mx: number, sk: number) =>
        Math.min(1, Math.pow(Math.max(0, (v - mn) / (mx - mn)), sk));

    // Cut Now: momentary — send rising edge then auto-reset after 100 ms
    function handleCutNow() {
        send('cutNow', 1);
        setTimeout(() => send('cutNow', 0), 100);
    }

    // normalized 0-1 → display value
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
        <span class="brand-sub">— REVERB·01</span>
      </div>
      <Toggle label="BYPASS" pill active={bypassed}
        on:change={e => { bypassed = e.detail.active; }} />
    </div>

    <div class="divider"></div>

    <div class="body">

      <!-- ══ TOP ROW ══ -->
      <div class="row-top">

        <!-- Presets -->
        <div class="side-card">
          <div class="card-title">Presets</div>
          <div class="presets-grid">
            {#each PRESET_NAMES as name, i}
              <button class="preset-btn" class:active={activePreset === i}
                title={name} on:click={() => applyPreset(i)}>
                {i + 1}
              </button>
            {/each}
          </div>
          {#if activePreset !== null}
            <div class="preset-name">{PRESET_NAMES[activePreset]}</div>
          {/if}
        </div>

        <!-- Camera -->
        <div class="camera-card">
          <Camera />
          <GestureController />
        </div>

        <!-- Gestures + quick access numbers -->
        <div class="side-card">
          <div class="card-title">Gestures</div>
          <div class="gesture-icons">
            <button class="icon-btn">🤙</button>
            <button class="icon-btn">✊</button>
          </div>
          <div class="thin-sep"></div>
          <div class="card-sub">Quick Access</div>
          <Number label="Gain"
            value={+dl($gainDb, -24, 12).toFixed(1)}
            min={-24} max={12} step={0.1} decimals={1} unit=" dB"
            on:change={e => send('gainDb', nl(e.detail.value, -24, 12))} />
          <Number label="Dry/Wet"
            value={+dl($dryWet, 0, 100).toFixed(0)}
            min={0} max={100} step={1} decimals={0} unit="%"
            on:change={e => send('dryWet', nl(e.detail.value, 0, 100))} />
        </div>

      </div>

      <!-- ══ BOTTOM ROW: 4 parameter panels ══ -->
      <div class="row-bottom">

        <!-- ─ REVERB ─ -->
        <div class="param-panel reverb-panel">
          <div class="card-title">Reverb</div>

          <div class="group-label">Performance</div>
          <div class="num-row perf-row">
            <Toggle label="SUSTAIN" led glow
              active={$sustainEnabled > 0.5}
              on:change={e => send('sustainEnabled', e.detail.active ? 1 : 0)} />
            <button class="cut-btn" on:click={handleCutNow}>CUT</button>
          </div>

          <div class="group-label">Core</div>
          <div class="num-row">
            <Number label="Decay"
              value={+ds($decay, 200, 60000, 0.3).toFixed(0)}
              min={200} max={60000} step={10} decimals={0} unit=" ms"
              on:change={e => send('decay', ns(e.detail.value, 200, 60000, 0.3))} />
            <Number label="Diffusion"
              value={+$diffusion.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('diffusion', e.detail.value)} />
            <Number label="Size"
              value={+$size.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('size', e.detail.value)} />
            <Number label="Damping"
              value={+$damping.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('damping', e.detail.value)} />
            <Number label="Feedback"
              value={+$feedback.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('feedback', e.detail.value)} />
            <Number label="Stereo"
              value={+dl($stereo, 0, 120).toFixed(0)}
              min={0} max={120} step={1} decimals={0} unit="°"
              on:change={e => send('stereo', nl(e.detail.value, 0, 120))} />
          </div>

          <div class="group-label">Mix</div>
          <div class="num-row">
            <Number label="Dry/Wet"
              value={+dl($dryWet, 0, 100).toFixed(0)}
              min={0} max={100} step={1} decimals={0} unit="%"
              on:change={e => send('dryWet', nl(e.detail.value, 0, 100))} />
            <Number label="Predelay"
              value={+ds($predelay, 0, 500, 0.5).toFixed(1)}
              min={0} max={500} step={1} decimals={1} unit=" ms"
              on:change={e => send('predelay', ns(e.detail.value, 0, 500, 0.5))} />
          </div>
        </div>

        <!-- ─ SATURATION ─ -->
        <div class="param-panel">
          <div class="card-title">Saturation</div>

          <div class="group-label">Gain</div>
          <div class="num-row">
            <Number label="Gain"
              value={+dl($gainDb, -24, 12).toFixed(1)}
              min={-24} max={12} step={0.1} decimals={1} unit=" dB"
              on:change={e => send('gainDb', nl(e.detail.value, -24, 12))} />
            <Number label="Trim"
              value={+dl($outputTrimDb, -12, 0).toFixed(1)}
              min={-12} max={0} step={0.1} decimals={1} unit=" dB"
              on:change={e => send('outputTrimDb', nl(e.detail.value, -12, 0))} />
          </div>

          <div class="group-label">Drive</div>
          <div class="num-row">
            <Number label="Drive"
              value={+$satDrive.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('satDrive', e.detail.value)} />
            <Number label="Mix"
              value={+dl($satMix, 0, 100).toFixed(0)}
              min={0} max={100} step={1} decimals={0} unit="%"
              on:change={e => send('satMix', nl(e.detail.value, 0, 100))} />
            <Number label="Tone"
              value={+$satTone.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('satTone', e.detail.value)} />
          </div>
        </div>

        <!-- ─ TIMBRE ─ -->
        <div class="param-panel">
          <div class="card-title">Timbre</div>
          <div class="num-row">
            <Number label="Cutoff"
              value={+ds($timbreCutoff, 20, 20000, 0.3).toFixed(0)}
              min={20} max={20000} step={10} decimals={0} unit=" Hz"
              on:change={e => send('timbreCutoff', ns(e.detail.value, 20, 20000, 0.3))} />
            <Number label="Resonance"
              value={+ds($timbreResonance, 0.5, 10, 0.5).toFixed(2)}
              min={0.5} max={10} step={0.01} decimals={2}
              on:change={e => send('timbreResonance', ns(e.detail.value, 0.5, 10, 0.5))} />
            <Number label="Pre-Drive"
              value={+dl($timbreDrivePre, 0, 12).toFixed(1)}
              min={0} max={12} step={0.1} decimals={1} unit=" dB"
              on:change={e => send('timbreDrivePre', nl(e.detail.value, 0, 12))} />
          </div>
        </div>

        <!-- ─ SHIMMER ─ -->
        <div class="param-panel">
          <div class="card-title">Shimmer</div>

          <div class="group-label">Vibrato</div>
          <div class="num-row">
            <Number label="Rate"
              value={+ds($vibratoRate, 0.1, 8, 0.5).toFixed(2)}
              min={0.1} max={8} step={0.01} decimals={2} unit=" Hz"
              on:change={e => send('vibratoRate', ns(e.detail.value, 0.1, 8, 0.5))} />
            <Number label="Depth"
              value={+$vibratoDepth.toFixed(2)}
              min={0} max={1} step={0.01} decimals={2}
              on:change={e => send('vibratoDepth', e.detail.value)} />
            <Number label="Fade In"
              value={+ds($vibratoFadeIn, 0, 500, 0.5).toFixed(0)}
              min={0} max={500} step={1} decimals={0} unit=" ms"
              on:change={e => send('vibratoFadeIn', ns(e.detail.value, 0, 500, 0.5))} />
          </div>

          <div class="group-label">Pitch Shifter</div>
          <div class="num-row">
            <Number label="Frequency"
              value={Math.round(110 + 1650 * Math.pow(Math.max(0, $pitchFrequency), 0.5))}
              min={110} max={1760} step={1} decimals={0} unit=" Hz"
              on:change={e => send('pitchFrequency', ns(e.detail.value, 110, 1760, 2))} />
            <Number label="Oct Step"
              value={Math.round($pitchOctaveStep)}
              min={0} max={1} step={1} decimals={0}
              on:change={e => send('pitchOctaveStep', e.detail.value)} />
          </div>
        </div>

      </div>
    </div>
  </div>
</main>

<Tooltip />

<style>
  :global(body) {
    margin: 0; padding: 0;
    user-select: none; cursor: default;
    background: #09070f;
    font-family: 'Jost', sans-serif;
    overflow-y: auto;
  }

  .scene {
    min-height: 100vh;
    display: flex; justify-content: center; align-items: flex-start;
    background:
      radial-gradient(ellipse at 25% 35%, rgba(160,140,220,0.1), transparent 55%),
      radial-gradient(ellipse at 75% 70%, rgba(200,140,255,0.07), transparent 50%),
      #09070f;
    padding: 1rem;
  }

  .plugin {
    width: 100%; max-width: 1400px;
    background: linear-gradient(135deg,
      rgba(255,255,255,0.07) 0%,
      rgba(200,190,240,0.04) 50%,
      rgba(160,140,220,0.06) 100%);
    backdrop-filter: blur(24px) saturate(160%);
    -webkit-backdrop-filter: blur(24px) saturate(160%);
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 20px;
    box-shadow: 0 24px 64px rgba(0,0,0,0.6), inset 0 1px 0 rgba(255,255,255,0.16);
    padding: 18px 20px 24px;
    transition: opacity 0.2s;
  }
  .plugin.bypassed { opacity: 0.4; pointer-events: none; }

  /* ── Header ── */
  .header {
    display: flex; align-items: center; justify-content: space-between;
    margin-bottom: 12px;
  }
  .brand { display: flex; align-items: baseline; gap: 8px; }
  .brand-name {
    font-size: 1.1rem; font-weight: 300; letter-spacing: 0.35em;
    text-transform: uppercase; color: rgba(230,220,255,0.88);
    text-shadow: 0 0 20px rgba(200,180,255,0.25);
  }
  .brand-sub {
    font-size: 0.6rem; font-weight: 200; letter-spacing: 0.25em;
    color: rgba(180,170,210,0.4);
  }

  .divider {
    height: 1px;
    background: linear-gradient(to right, transparent, rgba(255,255,255,0.09), transparent);
    margin: 0 -4px 14px;
  }

  /* ── Body ── */
  .body { display: flex; flex-direction: column; gap: 12px; }

  /* ── Top Row ── */
  .row-top { display: flex; gap: 10px; height: 260px; }

  .side-card {
    flex: 0 0 152px;
    background: rgba(255,255,255,0.05);
    border: 1px solid rgba(255,255,255,0.09);
    border-radius: 14px;
    padding: 12px;
    display: flex; flex-direction: column; gap: 8px;
    overflow: hidden;
  }

  .camera-card {
    flex: 1;
    height: 100%;           /* definite height so Camera's height:100% resolves */
    border-radius: 14px;
    overflow: hidden;
    border: 1px solid rgba(255,255,255,0.09);
    background: rgba(0,0,0,0.35);
  }

  .card-title {
    font-size: 0.52rem; font-weight: 300; letter-spacing: 0.3em;
    text-transform: uppercase; color: rgba(180,170,210,0.5);
    flex-shrink: 0;
  }

  .card-sub {
    font-size: 0.44rem; font-weight: 300; letter-spacing: 0.25em;
    text-transform: uppercase; color: rgba(180,170,210,0.3);
  }

  .thin-sep {
    height: 1px; background: rgba(255,255,255,0.07); flex-shrink: 0;
  }

  .presets-grid {
    display: grid; grid-template-columns: repeat(3, 1fr); gap: 5px;
    flex: 1; align-content: start;
  }
  .preset-btn {
    aspect-ratio: 1; border-radius: 50%;
    background: rgba(255,255,255,0.07);
    border: 1px solid rgba(255,255,255,0.12);
    color: rgba(200,190,230,0.65);
    font-family: 'Jost', sans-serif; font-size: 0.6rem; font-weight: 300;
    cursor: pointer; transition: background 0.15s;
  }
  .preset-btn:hover { background: rgba(255,110,40,0.18); color: rgba(255,170,100,0.9); }
  .preset-btn.active {
    background: rgba(255,110,40,0.28);
    border-color: rgba(255,110,40,0.55);
    color: rgba(255,180,110,1);
    box-shadow: 0 0 8px rgba(255,110,40,0.3);
  }
  .preset-name {
    font-family: 'Jost', sans-serif; font-size: 0.48rem; font-weight: 300;
    letter-spacing: 0.2em; text-transform: uppercase;
    color: rgba(255,140,60,0.55); text-align: center; margin-top: 2px;
  }

  .gesture-icons { display: flex; gap: 6px; flex-shrink: 0; }
  .icon-btn {
    padding: 5px 8px; border-radius: 8px;
    background: rgba(255,255,255,0.07);
    border: 1px solid rgba(255,255,255,0.11);
    font-size: 0.9rem; cursor: pointer;
    transition: background 0.15s;
  }
  .icon-btn:hover { background: rgba(255,110,40,0.15); }

  /* ── Bottom Row ── */
  .row-bottom {
    display: flex; gap: 10px; align-items: flex-start;
  }

  .param-panel {
    flex: 1;
    background: rgba(255,255,255,0.04);
    border: 1px solid rgba(255,255,255,0.08);
    border-radius: 14px;
    padding: 12px 14px;
    display: flex; flex-direction: column; gap: 8px;
    max-height: calc(100vh - 420px);
    min-height: 120px;
    overflow-y: auto;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.15) transparent;
  }
  .param-panel::-webkit-scrollbar { width: 4px; }
  .param-panel::-webkit-scrollbar-track { background: transparent; }
  .param-panel::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.15); border-radius: 2px; }

  .reverb-panel { flex: 1.6; }

  .group-label {
    font-size: 0.44rem; font-weight: 300; letter-spacing: 0.25em;
    text-transform: uppercase; color: rgba(180,170,210,0.3);
    margin-top: 2px;
  }

  .num-row {
    display: flex; flex-wrap: wrap; gap: 6px 10px;
  }

  .perf-row { align-items: center; }

  .cut-btn {
    padding: 3px 10px; border-radius: 6px;
    background: rgba(220, 60, 40, 0.18);
    border: 1px solid rgba(220, 60, 40, 0.4);
    color: rgba(255, 140, 120, 0.85);
    font-family: 'Jost', sans-serif; font-size: 0.52rem; font-weight: 300;
    letter-spacing: 0.2em; text-transform: uppercase;
    cursor: pointer; transition: background 0.1s, box-shadow 0.1s;
  }
  .cut-btn:hover { background: rgba(220, 60, 40, 0.35); }
  .cut-btn:active {
    background: rgba(220, 60, 40, 0.55);
    box-shadow: 0 0 8px rgba(220, 60, 40, 0.4);
  }
</style>
