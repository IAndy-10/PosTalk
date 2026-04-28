# UI Version History

---

## v0.2 — Proto Inspector (current)
**Branch/commit tag:** `ui/proto-inspector`

### Intent
Rapid parameter prototyping. No visual design — every parameter exposed as a scrollable/typeable number box for quick iteration and value selection.

### Layout
```
Header: POSTALK — REVERB·01  |  [BYPASS]

Row 1 (top, 260px):
  [Presets: 6 placeholder buttons]  [Camera]  [Gestures: icons + Gain/DryWet quick Numbers]

Row 2 (bottom, natural height):
  [Reverb flex:1.6]  [Saturation flex:1]  [Timbre flex:1]  [Vibrato flex:1]
```

### Component used
`Number.svelte` — double-click or scroll to change, shows real units (Hz, dB, ms, °, %).

### Parameters per panel
- **Reverb**: Decay, Diffusion, Size, Damping, Feedback, Stereo / DryWet, Predelay, Reflect, Diffuse / Crossover, Scale, Mode, Density, Smooth, HF Filter / Freeze, Flat, Cut / ER (On, Amount, Rate, Shape) / Chorus (On, Amount, Rate) / Input Filter (Lo Cut, Lo Freq, Hi Cut, Hi Freq)
- **Saturation**: Gain, Trim / Drive, Mix, Tone
- **Timbre**: Cutoff, Resonance, Pre-Drive
- **Vibrato**: Rate, Depth, Fade In

### Key files
- `WebUI/src/App.svelte` — full template

---

## v0.1 — Dark Glassmorphic (3-column)
**Branch/commit tag:** `ui/glassmorphic-3col`

### Intent
Performance/presentation UI. Tactile feel, dark ambient aesthetic for live use.

### Layout
```
Header: POSTALK — REVERB·01  |  [BYPASS]

3-column body:
  Left  (180px): Camera · GestureController · FilterGraph · Lo/Hi Cut toggles · Predelay knob
  Center (flex): 3×2 primary knob grid (Decay, Diffusion, Size, Damping, Feedback, Stereo)
                 Large DryWet prominent knob
                 3×5 secondary knob grid (Reflect, Diffuse, Scale, Gain, Trim, X-Over,
                                          Drive, Sat Mix, Tone, Cutoff, Resonance, Pre-Drive,
                                          Vib Rate, Vib Depth, Fade In)
  Right (200px): KnobDiscrete (Mode, Density, Smooth) · Selector (HF Filter)
                 Toggle rows (Freeze, Flat, Cut / ER Spin, Chorus)
                 Number inputs (ER Amount, ER Rate)

Footer: POSTALK DSP·01 — Spectral Reverb  |  status dot
```

### Design language
- Background: `#09070f` + purple radial gradients
- Panels: `rgba(255,255,255,0.06–0.09)` glass, `backdrop-filter: blur(24px)`
- Accent: `rgb(255,110,40)` orange
- Font: Jost 200/300/400
- Knob arc fill proportional to value, orange glow on active

### Components used
`Knob.svelte`, `Toggle.svelte`, `Selector.svelte`, `KnobDiscrete.svelte`, `Number.svelte`, `FilterGraph.svelte`, `Camera.svelte`, `GestureController.svelte`

### Key prop interfaces
- `Knob`: `value (0-1 norm)`, `min`, `max`, `unit`, `label`, `defaultValue`, `size`, `formatFn`, `prominent`, `showTick`
- `Toggle`: `label`, `icon`, `active`, `pill`, `glow`, `led`
- `Selector`: `label`, `options[]`, `selected`
- `KnobDiscrete`: `label`, `options[]`, `selected`
- `Number`: `label`, `value`, `min`, `max`, `step`, `unit`, `decimals`

### Skewed param display (formatFn patterns)
```ts
// Decay (200–60000 ms, skew 0.3)
formatFn={v => formatTime(200 + 59800 * Math.pow(v, 1/0.3))}
// Size (0.22–500, log)
formatFn={v => (0.22 * Math.pow(500.0/0.22, v)).toFixed(2)}
// X-Over (200–8000 Hz, skew 0.5)
formatFn={v => Math.round(200 + 7800 * Math.pow(v, 2)) + ' Hz'}
```

### Key files
- `WebUI/src/App.svelte` — full template
- `WebUI/src/components/Knob.svelte`
- `WebUI/src/components/Toggle.svelte`
- `WebUI/src/components/Selector.svelte`
- `WebUI/src/components/KnobDiscrete.svelte`
