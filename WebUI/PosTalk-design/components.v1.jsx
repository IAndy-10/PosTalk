/* PosTalk · atom components: Knob, VSlider, NumberField, LedToggle, LedStack */

const { useState, useEffect, useRef, useCallback } = React;

// ─── Knob ──────────────────────────────────────────────────────────
function Knob({
  label,
  value,         // 0..1 normalized
  onChange,
  display,       // string to show under label
  size = 'md',   // 'sm' | 'md' | 'lg'
  showArc = true,
}) {
  const [dragging, setDragging] = useState(false);
  const startRef = useRef({ y: 0, v: 0 });

  const angle = -135 + value * 270;

  const onMouseDown = (e) => {
    e.preventDefault();
    setDragging(true);
    startRef.current = { y: e.clientY, v: value };
    const onMove = (ev) => {
      const dy = startRef.current.y - ev.clientY;
      const nv = Math.max(0, Math.min(1, startRef.current.v + dy / 140));
      onChange(nv);
    };
    const onUp = () => {
      setDragging(false);
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  };

  const onWheel = (e) => {
    e.preventDefault();
    onChange(Math.max(0, Math.min(1, value - e.deltaY * 0.001)));
  };

  const onDoubleClick = () => onChange(0.5);

  // arc geometry
  const R = 26;
  const C = 32;
  const startAngle = -225;       // (top going clockwise from -135 → +135)
  const arcLen = 270;            // degrees of total travel
  const dashTotal = 2 * Math.PI * R;
  const fullDash = (arcLen / 360) * dashTotal;
  const valDash = (value * arcLen / 360) * dashTotal;

  return (
    <div className="knob-wrap">
      <div
        className={`knob ${size}`}
        onMouseDown={onMouseDown}
        onWheel={onWheel}
        onDoubleClick={onDoubleClick}
      >
        {showArc && (
          <svg className="knob-arc" viewBox="0 0 64 64">
            <circle
              cx={C} cy={C} r={R}
              fill="none"
              stroke="rgba(255,255,255,0.05)"
              strokeWidth="2"
              strokeDasharray={`${fullDash} ${dashTotal}`}
              strokeLinecap="round"
              transform="rotate(135 32 32)"
            />
            <circle
              cx={C} cy={C} r={R}
              fill="none"
              stroke="var(--accent)"
              strokeWidth="2"
              strokeDasharray={`${valDash} ${dashTotal}`}
              strokeLinecap="round"
              transform="rotate(135 32 32)"
              style={{
                filter: 'drop-shadow(0 0 3px var(--accent-glow))',
                opacity: 0.9,
              }}
            />
          </svg>
        )}
        <div className="knob-body" />
        <div className="knob-indicator">
          <div className="knob-tick" style={{ transform: `translateX(-50%) rotate(${angle}deg)` }} />
        </div>
      </div>
      <div className="knob-label">{label}</div>
      <div className="knob-val">{display}</div>
    </div>
  );
}

// ─── Vertical slider ───────────────────────────────────────────────
function VSlider({ label, value, onChange, display }) {
  const trackRef = useRef(null);

  const onMouseDown = (e) => {
    e.preventDefault();
    const rect = trackRef.current.getBoundingClientRect();
    const update = (clientY) => {
      const y = clientY - rect.top;
      const v = 1 - Math.max(0, Math.min(1, y / rect.height));
      onChange(v);
    };
    update(e.clientY);
    const onMove = (ev) => update(ev.clientY);
    const onUp = () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  };

  const ticks = Array.from({ length: 5 });

  return (
    <div className="vslider-wrap">
      <div className="vslider-label">{label}</div>
      <div className="vslider-track-wrap">
        <div className="vslider-track" ref={trackRef} onMouseDown={onMouseDown}>
          <div className="vslider-fill" style={{ height: `${value * 100}%` }} />
        </div>
        <div className="vslider-ticks">
          {ticks.map((_, i) => <div key={i} className="vslider-tick" />)}
        </div>
        <div
          className="vslider-handle"
          style={{ bottom: `${value * 100}%` }}
        />
      </div>
      <div className="vslider-val">{display}</div>
    </div>
  );
}

// ─── Number field ─────────────────────────────────────────────────
function NumberField({ label, value, onChange, display, muted = false }) {
  const startRef = useRef({ y: 0, v: 0 });

  const onMouseDown = (e) => {
    e.preventDefault();
    startRef.current = { y: e.clientY, v: value };
    const onMove = (ev) => {
      const dy = startRef.current.y - ev.clientY;
      const nv = Math.max(0, Math.min(1, startRef.current.v + dy / 200));
      onChange(nv);
    };
    const onUp = () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  };

  const onWheel = (e) => {
    e.preventDefault();
    onChange(Math.max(0, Math.min(1, value - e.deltaY * 0.001)));
  };

  return (
    <div className="numf">
      <div className="numf-label">{label}</div>
      <div
        className={`numf-pill ${muted ? 'muted' : ''}`}
        onMouseDown={onMouseDown}
        onWheel={onWheel}
      >
        {display}
      </div>
    </div>
  );
}

// ─── LED toggle (single pill) ─────────────────────────────────────
function LedToggle({ label, active, onClick }) {
  return (
    <div className={`led-toggle ${active ? 'active' : ''}`} onClick={onClick}>
      <div className="led-dot" />
      <span>{label}</span>
    </div>
  );
}

// ─── LED stack (group with header) ────────────────────────────────
function LedStack({ title, items, value, onChange }) {
  return (
    <div className="led-stack">
      <div className="led-stack-label">{title}</div>
      {items.map((opt) => (
        <LedToggle
          key={opt.value}
          label={opt.label}
          active={value === opt.value}
          onClick={() => onChange(opt.value)}
        />
      ))}
    </div>
  );
}

// ─── Lamp button (header: PWR, BYPASS, MONO) ──────────────────────
function Lamp({ label, on, onClick }) {
  return (
    <div className={`lamp ${on ? 'on' : ''}`} onClick={onClick}>
      <div className="lamp-bulb" />
      <div className="lamp-label">{label}</div>
    </div>
  );
}

Object.assign(window, { Knob, VSlider, NumberField, LedToggle, LedStack, Lamp });
