/* PosTalk · Camera card + Gesture mapping panel + simulated hand skeleton */

const { useState, useEffect, useRef } = React;

// ─── Camera card ──────────────────────────────────────────────────
function CameraCard({ onGesture, activeGesture }) {
  const videoRef = useRef(null);
  const canvasRef = useRef(null);
  const [status, setStatus] = useState('idle');  // 'idle' | 'requesting' | 'ready' | 'denied'
  const [error, setError] = useState('');
  const streamRef = useRef(null);

  const startCamera = async () => {
    setStatus('requesting');
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });
      streamRef.current = stream;
      if (videoRef.current) {
        videoRef.current.srcObject = stream;
        await videoRef.current.play().catch(() => {});
      }
      setStatus('ready');
    } catch (err) {
      setError(err.name || 'access denied');
      setStatus('denied');
    }
  };

  useEffect(() => {
    return () => {
      streamRef.current?.getTracks().forEach((t) => t.stop());
    };
  }, []);

  // Simulated hand skeleton when no camera available — animates softly.
  // When real camera is on, we still draw a faux skeleton overlay so the
  // mockup conveys what the real tracker shows. Visually accurate to brief.
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let raf;
    const resize = () => {
      const rect = canvas.getBoundingClientRect();
      canvas.width = rect.width;
      canvas.height = rect.height;
    };
    resize();
    const ro = new ResizeObserver(resize);
    ro.observe(canvas);

    const draw = (t) => {
      const w = canvas.width, h = canvas.height;
      ctx.clearRect(0, 0, w, h);

      // Only draw skeleton if camera is actually streaming
      if (status !== 'ready') {
        raf = requestAnimationFrame(draw);
        return;
      }

      const phase = t / 900;
      const cx = w * 0.5 + Math.sin(phase) * 30;
      const cy = h * 0.55 + Math.cos(phase * 1.3) * 16;
      const sc = Math.min(w, h) * 0.32;

      // 21 mediapipe-style landmarks
      const offsets = [
        [0, 0.7],          // wrist
        [-0.25, 0.4], [-0.35, 0.18], [-0.42, -0.02], [-0.48, -0.18],   // thumb
        [-0.12, 0.25], [-0.16, -0.05], [-0.18, -0.25], [-0.20, -0.42], // index
        [0.02, 0.22],   [0.02, -0.10], [0.02, -0.34], [0.02, -0.55],   // middle
        [0.16, 0.25],   [0.20, -0.05], [0.22, -0.25], [0.23, -0.42],   // ring
        [0.28, 0.32],   [0.34, 0.08], [0.38, -0.10], [0.40, -0.26],    // pinky
      ];

      const pts = offsets.map(([dx, dy]) => ({
        x: cx + dx * sc,
        y: cy + dy * sc,
      }));

      const conns = [
        [0,1],[1,2],[2,3],[3,4],
        [0,5],[5,6],[6,7],[7,8],
        [0,9],[9,10],[10,11],[11,12],
        [0,13],[13,14],[14,15],[15,16],
        [0,17],[17,18],[18,19],[19,20],
        [5,9],[9,13],[13,17],
      ];

      ctx.lineWidth = 1.5;
      ctx.lineCap = 'round';
      ctx.strokeStyle = 'rgba(255, 182, 72, 0.7)';
      ctx.shadowBlur = 6;
      ctx.shadowColor = 'rgba(255, 182, 72, 0.4)';
      for (const [a, b] of conns) {
        ctx.beginPath();
        ctx.moveTo(pts[a].x, pts[a].y);
        ctx.lineTo(pts[b].x, pts[b].y);
        ctx.stroke();
      }
      ctx.shadowBlur = 0;

      const TIPS = new Set([4, 8, 12, 16, 20]);
      for (let i = 0; i < pts.length; i++) {
        ctx.beginPath();
        ctx.arc(pts[i].x, pts[i].y, TIPS.has(i) ? 3 : 2, 0, Math.PI * 2);
        ctx.fillStyle = TIPS.has(i)
          ? 'rgba(255, 200, 110, 0.95)'
          : 'rgba(245, 240, 255, 0.85)';
        ctx.fill();
      }

      raf = requestAnimationFrame(draw);
    };
    raf = requestAnimationFrame(draw);
    return () => {
      cancelAnimationFrame(raf);
      ro.disconnect();
    };
  }, [status]);

  return (
    <div className="camera-card">
      <span className="cam-corner tl" />
      <span className="cam-corner tr" />
      <span className="cam-corner bl" />
      <span className="cam-corner br" />

      <div className="cam-status">
        <span className="dot" />
        {status === 'ready' ? 'tracking · 1 hand' : 'camera offline'}
      </div>

      <video
        ref={videoRef}
        className="cam-video"
        autoPlay
        muted
        playsInline
        style={{ display: status === 'ready' ? 'block' : 'none' }}
      />
      <canvas
        ref={canvasRef}
        className="cam-overlay"
        style={{ display: status === 'ready' ? 'block' : 'none' }}
      />

      {status !== 'ready' && (
        <div className="cam-placeholder">
          {status === 'requesting' ? (
            <>
              <div className="cam-spinner" />
              <div className="cam-placeholder-text">requesting camera…</div>
            </>
          ) : (
            <>
              <SilhouetteHand />
              <div className="cam-placeholder-text">
                hand-tracking standby<br />
                <span style={{ color: 'var(--text-xlo)' }}>
                  {status === 'denied' ? `· ${error} ·` : 'enable to map gestures to params'}
                </span>
              </div>
              <button className="cam-start-btn" onClick={startCamera}>
                {status === 'denied' ? 'retry camera' : 'enable camera'}
              </button>
            </>
          )}
        </div>
      )}

      <div className="cam-readout">
        <div className="cam-readout-group">
          <span>SIG <b>—</b></span>
          <span>FPS <b>{status === 'ready' ? '30' : '—'}</b></span>
          <span>LATENCY <b>{status === 'ready' ? '12ms' : '—'}</b></span>
        </div>
        <div className="cam-readout-group">
          <span>GESTURE <b>{activeGesture || 'none'}</b></span>
        </div>
      </div>
    </div>
  );
}

function SilhouetteHand() {
  return (
    <svg width="64" height="80" viewBox="0 0 64 80" style={{ opacity: 0.32 }}>
      <g stroke="var(--accent)" strokeWidth="1.4" strokeLinecap="round" fill="none">
        <line x1="32" y1="74" x2="12" y2="50" />
        <line x1="32" y1="74" x2="22" y2="32" />
        <line x1="32" y1="74" x2="32" y2="24" />
        <line x1="32" y1="74" x2="42" y2="32" />
        <line x1="32" y1="74" x2="52" y2="42" />
        <line x1="12" y1="50" x2="22" y2="32" />
        <line x1="22" y1="32" x2="32" y2="24" />
        <line x1="32" y1="24" x2="42" y2="32" />
        <line x1="42" y1="32" x2="52" y2="42" />
      </g>
      <g fill="var(--accent)">
        <circle cx="32" cy="74" r="2.2" />
        <circle cx="12" cy="50" r="1.6" />
        <circle cx="22" cy="32" r="1.6" />
        <circle cx="32" cy="24" r="1.6" />
        <circle cx="42" cy="32" r="1.6" />
        <circle cx="52" cy="42" r="1.6" />
      </g>
    </svg>
  );
}

// ─── Gesture Mapping panel ────────────────────────────────────────
function GestureMap({ activeGesture, onSimulate }) {
  const [hovered, setHovered] = useState(null);

  // Reflects the actual GestureController.svelte logic.
  const rows = [
    {
      id: 'up',
      glyph: <GestureIcon kind="single-up" />,
      trigger: 'Index ↑',
      sub: 'one hand · pointing up',
      target: ['Sustain', 'on'],
    },
    {
      id: 'down',
      glyph: <GestureIcon kind="single-down" />,
      trigger: 'Index ↓',
      sub: 'one hand · pointing down',
      target: ['Sustain', 'off'],
    },
    {
      id: 'pitch-up',
      glyph: <GestureIcon kind="both-up" />,
      trigger: 'Both ↑',
      sub: 'two hands · ramping up',
      target: ['Pitch Freq', '+'],
    },
    {
      id: 'pitch-down',
      glyph: <GestureIcon kind="both-down" />,
      trigger: 'Both ↓',
      sub: 'two hands · ramping down',
      target: ['Pitch Freq', '−'],
    },
  ];

  const detail = rows.find((r) => r.id === hovered);

  return (
    <>
      <div className="panel-title">Gestures</div>
      <div className="gmap-grid">
        {rows.map((r, i) => (
          <div
            key={i}
            className={`gmap-card ${hovered === r.id ? 'hovered' : ''} ${activeGesture === r.id ? 'active' : ''}`}
            onMouseEnter={() => { setHovered(r.id); onSimulate?.(r.id); }}
            onMouseLeave={() => { setHovered(null); onSimulate?.(null); }}
          >
            <div className="gmap-glyph">{r.glyph}</div>
            <div className="gmap-trigger">{r.trigger}</div>
          </div>
        ))}
      </div>

      <div className={`gmap-detail ${detail ? 'visible' : ''}`}>
        {detail ? (
          <>
            <div className="gmap-detail-sub">{detail.sub}</div>
            <div className="gmap-detail-map">
              <span className="gmap-detail-target">{detail.target[0]}</span>
              <span className="gmap-arrow">→</span>
              <span className="gmap-chip">{detail.target[1]}</span>
            </div>
          </>
        ) : (
          <div className="gmap-detail-placeholder">hover a gesture to inspect mapping</div>
        )}
      </div>
    </>
  );
}

function GestureIcon({ kind }) {
  // simple SVG glyphs for each gesture pattern
  const stroke = 'currentColor';
  if (kind === 'single-up')
    return (
      <svg width="22" height="22" viewBox="0 0 22 22" fill="none" stroke={stroke} strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round">
        <path d="M11 19 V8 M11 8 L8 11 M11 8 L14 11" />
        <circle cx="11" cy="5" r="1.4" fill={stroke} stroke="none" />
      </svg>
    );
  if (kind === 'single-down')
    return (
      <svg width="22" height="22" viewBox="0 0 22 22" fill="none" stroke={stroke} strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round">
        <path d="M11 3 V14 M11 14 L8 11 M11 14 L14 11" />
        <circle cx="11" cy="17" r="1.4" fill={stroke} stroke="none" />
      </svg>
    );
  if (kind === 'both-up')
    return (
      <svg width="22" height="22" viewBox="0 0 22 22" fill="none" stroke={stroke} strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
        <path d="M6 19 V9 M6 9 L4 11 M6 9 L8 11" />
        <path d="M16 19 V9 M16 9 L14 11 M16 9 L18 11" />
        <circle cx="6" cy="6" r="1.2" fill={stroke} stroke="none" />
        <circle cx="16" cy="6" r="1.2" fill={stroke} stroke="none" />
      </svg>
    );
  if (kind === 'both-down')
    return (
      <svg width="22" height="22" viewBox="0 0 22 22" fill="none" stroke={stroke} strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
        <path d="M6 3 V13 M6 13 L4 11 M6 13 L8 11" />
        <path d="M16 3 V13 M16 13 L14 11 M16 13 L18 11" />
        <circle cx="6" cy="16" r="1.2" fill={stroke} stroke="none" />
        <circle cx="16" cy="16" r="1.2" fill={stroke} stroke="none" />
      </svg>
    );
  return null;
}

Object.assign(window, { CameraCard, GestureMap });
