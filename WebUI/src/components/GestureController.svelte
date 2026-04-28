<script lang="ts">
    import { onMount, onDestroy } from 'svelte';
    import { get } from 'svelte/store';
    import { cameraStream } from '../stores/cameraStream';
    import { latestHands } from '../stores/landmarks';
    import { params, setParameterValue } from '../state/store';
    import { bridge } from '../bridge/bridge';
    import type { ParameterId } from '../types/parameters';

    // ── MediaPipe hand landmark indices ─────────────────────────
    // https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker
    const LM = {
        WRIST:       0,
        INDEX_PIP:   6,
        INDEX_TIP:   8,
        MIDDLE_PIP: 10,
        MIDDLE_TIP: 12,
        RING_PIP:   14,
        RING_TIP:   16,
        PINKY_PIP:  18,
        PINKY_TIP:  20,
    } as const;

    // ── Configuration ────────────────────────────────────────────
    // *** VELOCIDAD DEL CAMBIO WET/DRY ***
    // Cantidad que cambia dryWet por frame (rAF) mientras se mantiene el gesto.
    // A ~60 fps: RATE * 60 = cambio por segundo.
    //   0.006 → ±0.36/s  (rango completo en ~2.8 s)  ← valor original
    //   0.018 → ±1.08/s  (rango completo en ~0.9 s)  ← valor actual (3× más rápido)
    // Aumenta este valor para una respuesta más rápida; redúcelo para más suavidad.
    const RATE = 0.018;

    // Smoothing: require the gesture in ≥ SMOOTH_THRESHOLD of last SMOOTH_N frames.
    const SMOOTH_N         = 4;
    const SMOOTH_THRESHOLD = 2;

    // Minimum separation (y-axis, normalised) for reliable detection.
    const Y_WRIST_GAP = 0.10;  // index tip vs wrist
    const Y_PIP_GAP   = 0.02;  // index tip vs index PIP

    // ── Internal state ───────────────────────────────────────────
    let videoEl: HTMLVideoElement;
    let handLandmarker: any = null;
    let rafId: number | null = null;
    let status: 'idle' | 'loading' | 'ready' | 'error' = 'idle';

    let histUp   = new Array<boolean>(SMOOTH_N).fill(false);
    let histDown = new Array<boolean>(SMOOTH_N).fill(false);

    // Exported so parent can read current gesture if needed
    export let gesture: 'up' | 'down' | 'none' = 'none';

    // ── Helpers ──────────────────────────────────────────────────
    function sendDryWet(value: number) {
        const clamped = Math.max(0, Math.min(1, value));
        setParameterValue('dryWet' as ParameterId, clamped);
        bridge.sendParameterChange('dryWet' as ParameterId, clamped);
    }

    // ── Gesture classification ───────────────────────────────────
    // No handedness filter — MediaPipe's label for the user's right hand
    // varies depending on whether the underlying WebView flips the stream.
    // The geometry checks alone are sufficient to disambiguate.
    function classifyGesture(lm: any[]): 'up' | 'down' | 'none' {
        const w  = lm[LM.WRIST];
        const it = lm[LM.INDEX_TIP],  ip = lm[LM.INDEX_PIP];
        const mt = lm[LM.MIDDLE_TIP], mp = lm[LM.MIDDLE_PIP];
        const rt = lm[LM.RING_TIP],   rp = lm[LM.RING_PIP];
        const pt = lm[LM.PINKY_TIP],  pp = lm[LM.PINKY_PIP];

        // How many non-index fingers are curled (tip below its PIP in normal orientation)
        const curled =
            (mt.y > mp.y ? 1 : 0) +
            (rt.y > rp.y ? 1 : 0) +
            (pt.y > pp.y ? 1 : 0);

        // How many non-index fingers are "curled" in the inverted orientation
        // (tip above its PIP, because the whole hand is upside-down)
        const invertedCurled =
            (mt.y < mp.y ? 1 : 0) +
            (rt.y < rp.y ? 1 : 0) +
            (pt.y < pp.y ? 1 : 0);

        // ── Pointing UP ─────────────────────────────────────────
        // Index tip well above wrist, index extended (tip above PIP),
        // at least 2 of 3 other fingers curled downward.
        if (
            it.y < w.y  - Y_WRIST_GAP &&
            it.y < ip.y - Y_PIP_GAP   &&
            curled >= 2
        ) return 'up';

        // ── Pointing DOWN (hand inverted) ────────────────────────
        // Index tip well below wrist, at least 2 of 3 other fingers
        // curled upward (inverted fist).
        if (
            it.y > w.y  + Y_WRIST_GAP &&
            it.y > ip.y + Y_PIP_GAP   &&
            invertedCurled >= 2
        ) return 'down';

        return 'none';
    }

    function smooth(raw: 'up' | 'down' | 'none'): 'up' | 'down' | 'none' {
        histUp.shift();   histUp.push(raw === 'up');
        histDown.shift(); histDown.push(raw === 'down');

        const upVotes   = histUp.filter(Boolean).length;
        const downVotes = histDown.filter(Boolean).length;

        if (upVotes   >= SMOOTH_THRESHOLD) return 'up';
        if (downVotes >= SMOOTH_THRESHOLD) return 'down';
        return 'none';
    }

    // ── Detection loop ───────────────────────────────────────────
    function loop() {
        if (status !== 'ready' || !handLandmarker || videoEl.readyState < 2) {
            rafId = requestAnimationFrame(loop);
            return;
        }

        const results = handLandmarker.detectForVideo(videoEl, performance.now());

        // Publish landmarks so Camera.svelte can draw the skeleton overlay.
        latestHands.set(results.landmarks.length > 0 ? results : null);

        let raw: 'up' | 'down' | 'none' = 'none';
        if (results.landmarks.length > 0) {
            raw = classifyGesture(results.landmarks[0]);
        }

        gesture = smooth(raw);

        if (gesture !== 'none') {
            const cur  = get(params.dryWet);
            const next = cur + (gesture === 'up' ? RATE : -RATE);
            if (next !== cur) sendDryWet(next);
        }

        rafId = requestAnimationFrame(loop);
    }

    // ── MediaPipe initialisation ─────────────────────────────────
    async function initMediaPipe(stream: MediaStream) {
        if (status !== 'idle') return;
        status = 'loading';
        try {
            // Dynamic import keeps MediaPipe out of the critical-path bundle.
            // The WASM is fetched from jsDelivr at runtime (not inlined).
            const { HandLandmarker, FilesetResolver } = await import(
                '@mediapipe/tasks-vision'
            );

            const fsr = await FilesetResolver.forVisionTasks(
                'https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.3/wasm'
            );

            handLandmarker = await HandLandmarker.createFromOptions(fsr, {
                baseOptions: {
                    modelAssetPath:
                        'https://storage.googleapis.com/mediapipe-models/' +
                        'hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task',
                    // CPU is more reliable inside WebKit-based WebViews (JUCE).
                    // Can be switched to 'GPU' if performance is a concern.
                    delegate: 'CPU',
                },
                runningMode: 'VIDEO',
                numHands: 1,
            });

            // Attach the shared stream to our hidden video element.
            videoEl.srcObject = stream;
            await new Promise<void>(res => { videoEl.onloadeddata = () => res(); });
            await videoEl.play();

            status = 'ready';
            rafId = requestAnimationFrame(loop);
        } catch (err) {
            console.warn('[GestureController] MediaPipe init failed:', err);
            status = 'error';
        }
    }

    // ── Lifecycle ────────────────────────────────────────────────
    onMount(() => {
        // Subscribe returns its unsubscribe fn; returning it from onMount
        // means Svelte calls it automatically on component destroy.
        return cameraStream.subscribe(stream => {
            if (stream && status === 'idle') {
                initMediaPipe(stream);
            }
        });
    });

    onDestroy(() => {
        if (rafId !== null) cancelAnimationFrame(rafId);
        latestHands.set(null);
        // HandLandmarker.close() releases the WASM resources.
        try { handLandmarker?.close(); } catch (_) {}
    });
</script>

<!--
    Hidden video element that MediaPipe reads from.
    The visible camera feed in Camera.svelte is separate (same stream, two <video>s).
    Future: add a <canvas> overlay here to draw the hand skeleton.
-->
<video bind:this={videoEl} style="display:none" muted playsinline></video>

<!-- Compact gesture status indicator shown in the Output panel -->
<div class="gesture-wrap">
    <div class="gesture-icon" class:active={gesture !== 'none'}>
        {#if status === 'loading'}
            <span class="dot loading" title="Loading gesture model…"></span>
        {:else if status === 'error'}
            <span class="dot error"   title="Gesture model unavailable"></span>
        {:else if gesture === 'up'}
            <span class="arrow" title="More wet">↑</span>
        {:else if gesture === 'down'}
            <span class="arrow" title="More dry">↓</span>
        {:else}
            <span class="dot ready" class:on={status === 'ready'} title="Gesture control ready"></span>
        {/if}
    </div>
    <span class="gesture-label">
        {gesture === 'up' ? 'wet ↑' : gesture === 'down' ? 'dry ↑' : 'gesture'}
    </span>
</div>

<style>
    .gesture-wrap {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 4px;
    }

    .gesture-icon {
        width: 26px;
        height: 26px;
        border-radius: 50%;
        background: #e8e0d4;
        box-shadow:
            3px 3px 8px rgba(160, 140, 110, 0.35),
            -3px -3px 7px rgba(255, 252, 245, 0.85);
        display: flex;
        align-items: center;
        justify-content: center;
        transition: box-shadow 0.2s ease;
    }

    .gesture-icon.active {
        box-shadow:
            2px 2px 5px rgba(160, 140, 110, 0.3),
            -2px -2px 5px rgba(255, 252, 245, 0.8),
            inset 2px 2px 5px rgba(150, 130, 100, 0.18),
            inset -1px -1px 4px rgba(255, 252, 240, 0.6);
    }

    .dot {
        width: 6px;
        height: 6px;
        border-radius: 50%;
        background: rgba(150, 130, 100, 0.2);
        transition: background 0.3s;
    }
    .dot.on      { background: rgba(0, 200, 180, 0.35); }
    .dot.loading { animation: pulse 1.1s ease-in-out infinite; }
    .dot.error   { background: rgba(200, 80, 60, 0.45); }

    .arrow {
        font-size: 0.7rem;
        line-height: 1;
        color: rgba(0, 200, 180, 0.85);
        text-shadow: 0 0 8px rgba(0, 200, 180, 0.45);
    }

    .gesture-label {
        font-family: 'DM Sans', sans-serif;
        font-size: 0.48rem;
        font-weight: 300;
        letter-spacing: 0.15em;
        text-transform: uppercase;
        color: rgba(140, 115, 82, 0.4);
    }

    @keyframes pulse {
        0%, 100% { background: rgba(0, 200, 180, 0.1); }
        50%       { background: rgba(0, 200, 180, 0.5); }
    }
</style>
