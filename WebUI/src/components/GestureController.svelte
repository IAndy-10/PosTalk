<script lang="ts">
    import { onMount, onDestroy } from 'svelte';
    import { get } from 'svelte/store';
    import { cameraStream } from '../stores/cameraStream';
    import { latestHands } from '../stores/landmarks';
    import { params, setParameterValue } from '../state/store';
    import { bridge } from '../bridge/bridge';
    import type { ParameterId } from '../types/parameters';

    // ── MediaPipe hand landmark indices ─────────────────────────
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
    // Single-hand smoothing: gesture must appear in ≥ THRESHOLD of last N frames
    const SMOOTH_N         = 4;
    const SMOOTH_THRESHOLD = 2;

    // Minimum Y separation for reliable single-hand detection
    const Y_WRIST_GAP = 0.10;
    const Y_PIP_GAP   = 0.02;

    // Both-hands pitch control:
    // Multiplier on average wrist Y velocity → normalized pitchFrequency change per frame.
    // At ~60 fps with a typical hand rise of 0.02 Y/frame: 0.02 * 2.5 = 0.05/frame → full range in ~1s.
    const PITCH_RATE     = 2.5;
    const PITCH_DEADZONE = 0.003;  // ignore tiny Y jitter when hands are still

    // ── Internal state ───────────────────────────────────────────
    let videoEl: HTMLVideoElement;
    let handLandmarker: any = null;
    let rafId: number | null = null;
    let status: 'idle' | 'loading' | 'ready' | 'error' = 'idle';

    let histUp   = new Array<boolean>(SMOOTH_N).fill(false);
    let histDown = new Array<boolean>(SMOOTH_N).fill(false);

    // Exported so parent can read current gesture
    export let gesture: 'up' | 'down' | 'pitch' | 'none' = 'none';

    // Edge-detect: only send sustain/freeze on change, not every frame
    let lastSustain = false;
    let lastFreeze  = false;

    // Previous average wrist Y — used to compute Y velocity for pitch control
    let prevAvgY: number | null = null;

    // ── Helpers ──────────────────────────────────────────────────
    function send(id: ParameterId, value: number) {
        setParameterValue(id, value);
        bridge.sendParameterChange(id, value);
    }

    // ── Single-hand gesture classification ───────────────────────
    function classifyGesture(lm: any[]): 'up' | 'down' | 'none' {
        const w  = lm[LM.WRIST];
        const it = lm[LM.INDEX_TIP],  ip = lm[LM.INDEX_PIP];
        const mt = lm[LM.MIDDLE_TIP], mp = lm[LM.MIDDLE_PIP];
        const rt = lm[LM.RING_TIP],   rp = lm[LM.RING_PIP];
        const pt = lm[LM.PINKY_TIP],  pp = lm[LM.PINKY_PIP];

        // Fingers curled in normal (upright) orientation
        const curled =
            (mt.y > mp.y ? 1 : 0) +
            (rt.y > rp.y ? 1 : 0) +
            (pt.y > pp.y ? 1 : 0);

        // Fingers curled in inverted (upside-down) orientation
        const invertedCurled =
            (mt.y < mp.y ? 1 : 0) +
            (rt.y < rp.y ? 1 : 0) +
            (pt.y < pp.y ? 1 : 0);

        // Index pointing up → Sustain
        if (it.y < w.y - Y_WRIST_GAP && it.y < ip.y - Y_PIP_GAP && curled >= 2)
            return 'up';

        // Index pointing down (inverted hand) → Freeze
        if (it.y > w.y + Y_WRIST_GAP && it.y > ip.y + Y_PIP_GAP && invertedCurled >= 2)
            return 'down';

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

        const results  = handLandmarker.detectForVideo(videoEl, performance.now());
        const numHands = results.landmarks.length;

        // Publish landmarks so Camera.svelte can draw the skeleton overlay
        latestHands.set(numHands > 0 ? results : null);

        if (numHands >= 2) {
            // ── Both hands: pitch frequency control ─────────────────
            // Release any held sustain/freeze before entering pitch mode
            if (lastSustain) { send('sustainEnabled', 0); lastSustain = false; }
            if (lastFreeze)  { send('freeze', 0);         lastFreeze  = false; }

            // Average wrist Y across both hands (camera Y increases downward)
            const avgY = (results.landmarks[0][LM.WRIST].y +
                          results.landmarks[1][LM.WRIST].y) / 2;

            if (prevAvgY !== null) {
                // prevAvgY - avgY > 0 means hands moved up → increase frequency
                const delta = prevAvgY - avgY;
                if (Math.abs(delta) > PITCH_DEADZONE) {
                    const cur  = get(params.pitchFrequency);
                    const next = Math.max(0, Math.min(1, cur + delta * PITCH_RATE));
                    if (Math.abs(next - cur) > 0.0001) send('pitchFrequency', next);
                }
            }
            prevAvgY = avgY;
            gesture  = 'pitch';

        } else if (numHands === 1) {
            // ── One hand: sustain (index up) or freeze (index down) ──
            prevAvgY = null;

            const g = smooth(classifyGesture(results.landmarks[0]));
            gesture  = g;

            const wantSustain = g === 'up';
            const wantFreeze  = g === 'down';

            if (wantSustain !== lastSustain) {
                send('sustainEnabled', wantSustain ? 1 : 0);
                lastSustain = wantSustain;
            }
            if (wantFreeze !== lastFreeze) {
                send('freeze', wantFreeze ? 1 : 0);
                lastFreeze = wantFreeze;
            }

        } else {
            // ── No hands: release everything ─────────────────────────
            prevAvgY = null;
            if (lastSustain) { send('sustainEnabled', 0); lastSustain = false; }
            if (lastFreeze)  { send('freeze', 0);         lastFreeze  = false; }
            gesture = 'none';
        }

        rafId = requestAnimationFrame(loop);
    }

    // ── MediaPipe initialisation ─────────────────────────────────
    async function initMediaPipe(stream: MediaStream) {
        if (status !== 'idle') return;
        status = 'loading';
        try {
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
                    delegate: 'CPU',
                },
                runningMode: 'VIDEO',
                numHands: 2,
            });

            videoEl.srcObject = stream;
            await new Promise<void>(res => { videoEl.onloadeddata = () => res(); });
            await videoEl.play();

            status = 'ready';
            rafId  = requestAnimationFrame(loop);
        } catch (err) {
            console.warn('[GestureController] MediaPipe init failed:', err);
            status = 'error';
        }
    }

    // ── Lifecycle ────────────────────────────────────────────────
    onMount(() => {
        return cameraStream.subscribe(stream => {
            if (stream && status === 'idle') initMediaPipe(stream);
        });
    });

    onDestroy(() => {
        if (rafId !== null) cancelAnimationFrame(rafId);
        latestHands.set(null);
        try { handLandmarker?.close(); } catch (_) {}
    });
</script>

<!-- Hidden video element that MediaPipe reads from -->
<video bind:this={videoEl} style="display:none" muted playsinline></video>

<div class="gesture-wrap">
    <div class="gesture-icon" class:active={gesture !== 'none'}>
        {#if status === 'loading'}
            <span class="dot loading" title="Loading gesture model…"></span>
        {:else if status === 'error'}
            <span class="dot error" title="Gesture model unavailable"></span>
        {:else if gesture === 'up'}
            <span class="arrow" title="Sustain">↑</span>
        {:else if gesture === 'down'}
            <span class="arrow" title="Freeze">↓</span>
        {:else if gesture === 'pitch'}
            <span class="arrow pitch" title="Pitch control">⇕</span>
        {:else}
            <span class="dot ready" class:on={status === 'ready'} title="Gesture control ready"></span>
        {/if}
    </div>
    <span class="gesture-label">
        {gesture === 'up'    ? 'sustain' :
         gesture === 'down'  ? 'freeze'  :
         gesture === 'pitch' ? 'pitch'   : 'gesture'}
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

    .arrow.pitch {
        color: rgba(180, 120, 255, 0.85);
        text-shadow: 0 0 8px rgba(180, 120, 255, 0.45);
    }

    .gesture-label {
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
