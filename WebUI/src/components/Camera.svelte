<script lang="ts">
    import { onMount, onDestroy } from 'svelte';
    import { cameraStream } from '../stores/cameraStream';
    import { latestHands }  from '../stores/landmarks';
    import type { NormalizedLandmark } from '../stores/landmarks';

    let videoEl:  HTMLVideoElement;
    let canvasEl: HTMLCanvasElement;
    let stream: MediaStream | null = null;
    let error  = '';
    let active = false;

    // ── Hand skeleton connections (MediaPipe 21-landmark topology) ──
    const CONNECTIONS: [number, number][] = [
        [0,1],[1,2],[2,3],[3,4],          // thumb
        [0,5],[5,6],[6,7],[7,8],          // index
        [0,9],[9,10],[10,11],[11,12],     // middle
        [0,13],[13,14],[14,15],[15,16],   // ring
        [0,17],[17,18],[18,19],[19,20],   // pinky
        [5,9],[9,13],[13,17],             // palm knuckle bar
    ];

    // Finger-tip indices — drawn with a brighter accent dot.
    const TIPS = new Set([4, 8, 12, 16, 20]);

    function drawSkeleton(lm: NormalizedLandmark[]) {
        if (!canvasEl) return;
        const ctx = canvasEl.getContext('2d');
        if (!ctx) return;
        const { width: w, height: h } = canvasEl;

        ctx.clearRect(0, 0, w, h);

        // ── Connections ─────────────────────────────────────────
        ctx.lineWidth   = 1.5;
        ctx.lineCap     = 'round';
        ctx.strokeStyle = 'rgba(0, 200, 180, 0.65)';

        for (const [a, b] of CONNECTIONS) {
            ctx.beginPath();
            ctx.moveTo(lm[a].x * w, lm[a].y * h);
            ctx.lineTo(lm[b].x * w, lm[b].y * h);
            ctx.stroke();
        }

        // ── Landmark dots ────────────────────────────────────────
        for (let i = 0; i < lm.length; i++) {
            const px = lm[i].x * w;
            const py = lm[i].y * h;
            const r  = TIPS.has(i) ? 3 : 2;

            ctx.beginPath();
            ctx.arc(px, py, r, 0, Math.PI * 2);
            ctx.fillStyle = TIPS.has(i)
                ? 'rgba(0, 200, 180, 0.95)'
                : 'rgba(255, 250, 240, 0.85)';
            ctx.fill();
        }
    }

    function clearCanvas() {
        if (!canvasEl) return;
        const ctx = canvasEl.getContext('2d');
        ctx?.clearRect(0, 0, canvasEl.width, canvasEl.height);
    }

    // ── Camera ───────────────────────────────────────────────────
    async function startCamera() {
        if (!navigator.mediaDevices?.getUserMedia) {
            error = 'No mediaDevices API';
            return;
        }
        try {
            stream = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });
            videoEl.srcObject = stream;
            active = true;
            error  = '';
            cameraStream.set(stream);
        } catch (e: any) {
            error  = e.name ?? 'Unknown';
            active = false;
        }
    }

    function stopCamera() {
        stream?.getTracks().forEach(t => t.stop());
        stream = null;
        active = false;
        if (videoEl) videoEl.srcObject = null;
        cameraStream.set(null);
    }

    // ── Lifecycle ────────────────────────────────────────────────
    onMount(() => {
        startCamera();

        // Subscribe to hand landmarks and redraw whenever they change.
        const unsubHands = latestHands.subscribe(data => {
            if (data && data.landmarks.length > 0) {
                drawSkeleton(data.landmarks[0]);
            } else {
                clearCanvas();
            }
        });

        return unsubHands;
    });

    onDestroy(() => { stopCamera(); });
</script>

<div class="camera-wrap">
    <!-- svelte-ignore a11y-media-has-caption -->
    <video
        bind:this={videoEl}
        class="cam-video"
        class:hidden={!active}
        autoplay
        playsinline
        muted
    ></video>

    <!--
        Skeleton overlay — same mirror transform as the video so landmarks
        align perfectly with the reflected image the user sees.
        Internal resolution 320×240 is enough for crisp lines at display size.
    -->
    <canvas
        bind:this={canvasEl}
        class="cam-overlay"
        width="320"
        height="240"
    ></canvas>

    {#if !active}
        <div class="cam-placeholder">
            {#if error}
                <span class="cam-error">{error}</span>
            {:else}
                <span class="cam-spinner"></span>
            {/if}
        </div>
    {/if}
</div>

<style>
    .camera-wrap {
        width: 100%;
        aspect-ratio: 4 / 3;
        max-height: 120px;
        border-radius: 10px;
        overflow: hidden;
        background: #1a1614;
        box-shadow:
            inset 4px 4px 10px rgba(100,80,55,0.35),
            inset -3px -3px 8px rgba(255,250,240,0.06);
        position: relative;
        flex-shrink: 0;
    }

    .cam-video {
        width: 100%;
        height: 100%;
        object-fit: cover;
        display: block;
        transform: scaleX(-1); /* mirror */
    }

    .cam-overlay {
        position: absolute;
        inset: 0;
        width: 100%;
        height: 100%;
        transform: scaleX(-1); /* mirror — keeps skeleton aligned with video */
        pointer-events: none;
    }

    .hidden { display: none; }

    .cam-placeholder {
        position: absolute;
        inset: 0;
        display: flex;
        align-items: center;
        justify-content: center;
    }

    .cam-error {
        font-size: 0.48rem;
        letter-spacing: 0.15em;
        text-transform: uppercase;
        color: rgba(200,170,130,0.45);
    }

    /* simple spinning ring */
    .cam-spinner {
        width: 14px;
        height: 14px;
        border: 1.5px solid rgba(200,170,130,0.15);
        border-top-color: rgba(0,200,180,0.5);
        border-radius: 50%;
        animation: spin 0.9s linear infinite;
    }

    @keyframes spin {
        to { transform: rotate(360deg); }
    }
</style>
