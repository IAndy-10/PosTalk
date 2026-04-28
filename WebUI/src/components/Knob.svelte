<script lang="ts">
  import { createEventDispatcher } from 'svelte';
  import { tweened } from 'svelte/motion';
  import { linear } from 'svelte/easing';
  import { tooltipStore } from '../stores/tooltip';

  export let label       = 'GAIN';
  export let value       = 0.5;   // normalized 0–1
  export let min         = 0;
  export let max         = 100;
  export let unit        = '';
  export let defaultValue: number | null = null;
  export let size: 'normal' | 'large' = 'normal';
  export let formatFn: ((v: number) => string) | null = null;
  export let prominent   = false;
  export let showTick    = false;

  const dispatch = createEventDispatcher();

  let dragging = false;
  let startY   = 0;
  let startVal = value;

  const MIN_ANGLE = -135;
  const MAX_ANGLE =  135;

  const tween = tweened(value, { duration: 80, easing: linear });
  $: tween.set(value, { duration: dragging ? 0 : 80 });

  $: angle      = MIN_ANGLE + $tween * (MAX_ANGLE - MIN_ANGLE);
  $: rawDisplay = min + $tween * (max - min);
  $: displayVal = formatFn ? formatFn(rawDisplay) : rawDisplay.toFixed(1);

  function onMouseDown(e) {
    dragging = true; startY = e.clientY; startVal = value;
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup',   onUp);
  }
  function onMove(e) {
    if (!dragging) return;
    value = Math.min(1, Math.max(0, startVal + (startY - e.clientY) / 120));
    dispatch('change', { value });
  }
  function onUp() {
    dragging = false;
    dispatch('change', { value });
    window.removeEventListener('mousemove', onMove);
    window.removeEventListener('mouseup',   onUp);
  }
  function onWheel(e) {
    e.preventDefault();
    value = Math.min(1, Math.max(0, value - e.deltaY * 0.001));
    dispatch('change', { value });
  }
  function onDblClick(e) {
    e.preventDefault();
    value = defaultValue !== null ? defaultValue : 0.5;
    dispatch('change', { value });
  }

  let ttTimer;
  function onMouseEnter(e) {
    ttTimer = setTimeout(() => tooltipStore.set({
      visible: true, x: e.clientX + 12, y: e.clientY - 52,
      name: label, value: `${displayVal}${unit}`, range: `${min} – ${max}${unit}`,
    }), 600);
  }
  function onMouseLeave() {
    clearTimeout(ttTimer);
    tooltipStore.update(s => ({ ...s, visible: false }));
  }
  function onMouseMove(e) {
    tooltipStore.update(s => s.visible ? { ...s, x: e.clientX + 12, y: e.clientY - 52 } : s);
  }

  let ctxVisible = false, ctxX = 0, ctxY = 0;
  function onContextMenu(e) {
    e.preventDefault(); ctxX = e.clientX; ctxY = e.clientY; ctxVisible = true;
    setTimeout(() => window.addEventListener('mousedown', closeCtx, { once: true }), 0);
  }
  function closeCtx() { ctxVisible = false; }
  function ctxSetDefault() {
    value = defaultValue !== null ? defaultValue : 0.5;
    dispatch('change', { value }); closeCtx();
  }
  async function ctxCopy() {
    await navigator.clipboard.writeText(`${displayVal}${unit}`).catch(() => {});
    closeCtx();
  }
  async function ctxPaste() {
    try {
      const t = await navigator.clipboard.readText();
      const p = parseFloat(t);
      if (!isNaN(p)) { value = Math.min(1, Math.max(0, (p - min) / (max - min))); dispatch('change', { value }); }
    } catch {}
    closeCtx();
  }
</script>

<div class="wrap"
  on:mouseenter={onMouseEnter} on:mouseleave={onMouseLeave}
  on:mousemove={onMouseMove}   on:contextmenu={onContextMenu}
>
  <div
    class="knob" class:pressed={dragging} class:large={size === 'large'}
    on:mousedown={onMouseDown} on:wheel|passive={onWheel} on:dblclick={onDblClick}
    role="slider" tabindex="0"
    aria-label="{label}: {displayVal}{unit}"
    aria-valuenow={rawDisplay} aria-valuemin={min} aria-valuemax={max}
  >
    {#if showTick}
      <svg class="arc tick-layer" viewBox="0 0 64 64" aria-hidden="true">
        <line x1="32" y1="3" x2="32" y2="9"
          stroke="rgba(255,110,40,0.5)" stroke-width="2" stroke-linecap="round"/>
      </svg>
    {/if}

    <!-- Glow halo proportional to value -->
    <div class="glow-ring" style="opacity: {0.1 + value * 0.55}"></div>

    <!-- Value arc -->
    <svg class="arc" viewBox="0 0 64 64" aria-hidden="true">
      <circle cx="32" cy="32" r="26" fill="none"
        stroke="rgba(255,255,255,0.07)" stroke-width="2.5"
        stroke-dasharray="130 41" stroke-dashoffset="10" stroke-linecap="round"
        transform="rotate(135 32 32)" />
      <circle cx="32" cy="32" r="26" fill="none"
        stroke="rgba(255,110,40,0.75)" stroke-width="2.5"
        stroke-dasharray="{value * 130} 171" stroke-dashoffset="10" stroke-linecap="round"
        transform="rotate(135 32 32)" />
    </svg>

    <!-- Rotating body -->
    <div class="body" style="transform: rotate({angle}deg)">
      <div class="dot"></div>
    </div>
  </div>

  <div class="label">{label}</div>
  <div class="val" class:val-prominent={prominent}>{displayVal}{unit}</div>

  {#if ctxVisible}
    <div class="ctx-menu" style="left:{ctxX}px;top:{ctxY}px;" role="menu">
      <button class="ctx-item" on:click={ctxSetDefault}>Set to default</button>
      <button class="ctx-item" on:click={ctxCopy}>Copy value</button>
      <button class="ctx-item" on:click={ctxPaste}>Paste value</button>
    </div>
  {/if}
</div>

<style>
  .wrap {
    display: flex; flex-direction: column; align-items: center;
    gap: 6px; user-select: none; position: relative;
  }

  .knob {
    position: relative; width: 64px; height: 64px; cursor: ns-resize;
  }
  .knob.large { width: 83px; height: 83px; }

  .glow-ring {
    position: absolute; inset: 6px; border-radius: 50%;
    background: radial-gradient(circle, rgba(255,110,40,0.35), transparent 70%);
    pointer-events: none; transition: opacity 0.15s;
  }

  .arc {
    position: absolute; inset: 0; width: 100%; height: 100%; pointer-events: none;
  }
  .tick-layer { z-index: 1; }

  .body {
    position: absolute; inset: 8px; border-radius: 50%;
    background: radial-gradient(circle at 35% 30%, rgba(255,255,255,0.16), rgba(160,150,210,0.05));
    backdrop-filter: blur(8px);
    border: 1px solid rgba(255,255,255,0.18);
    box-shadow: 0 4px 16px rgba(0,0,0,0.45), inset 0 1px 0 rgba(255,255,255,0.2);
    display: flex; align-items: flex-start; justify-content: center;
    padding-top: 5px; z-index: 2; position: relative;
    transition: box-shadow 0.1s;
  }
  .knob.large .body { inset: 10px; padding-top: 7px; }
  .knob.pressed .body {
    box-shadow: 0 2px 8px rgba(0,0,0,0.55), 0 0 16px rgba(255,110,40,0.3),
      inset 0 1px 0 rgba(255,255,255,0.25);
  }

  .dot {
    width: 4px; height: 4px; border-radius: 50%;
    background: rgba(255,130,60,0.95);
    box-shadow: 0 0 6px rgba(255,100,40,0.9);
  }
  .knob.large .dot { width: 5px; height: 5px; }

  .label {
    font-family: 'Jost', sans-serif; font-size: 0.56rem; font-weight: 300;
    letter-spacing: 0.2em; text-transform: uppercase; color: rgba(180,170,210,0.6);
  }
  .val {
    font-family: 'Jost', sans-serif; font-size: 0.56rem; font-weight: 200;
    color: rgba(255,150,80,0.75); letter-spacing: 0.08em;
  }
  .val.val-prominent {
    font-size: 0.78rem; font-weight: 300; color: rgba(255,160,90,0.92);
  }

  .ctx-menu {
    position: fixed; z-index: 9998;
    background: rgba(14,10,24,0.97); border: 1px solid rgba(255,255,255,0.1);
    border-radius: 10px; padding: 6px; display: flex; flex-direction: column;
    gap: 2px; box-shadow: 0 8px 32px rgba(0,0,0,0.55);
    min-width: 120px; backdrop-filter: blur(12px);
  }
  .ctx-item {
    font-family: 'Jost', sans-serif; font-size: 0.6rem; font-weight: 300;
    letter-spacing: 0.08em; color: rgba(200,190,230,0.7);
    background: transparent; border: none; border-radius: 6px;
    padding: 6px 10px; text-align: left; cursor: pointer; transition: background 0.12s;
  }
  .ctx-item:hover { background: rgba(255,110,40,0.15); color: rgba(255,170,100,0.9); }
</style>
