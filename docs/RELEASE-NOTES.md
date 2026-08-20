# Release Notes

## v0.3.0 (Initial Release)

**Date:** August 19, 2026

### Features
- 8-line Feedback Delay Network (FDN) reverb with Hadamard feedback matrix and prime-spaced delay lines
- Early reflections — 8-tap parallel delay with spin modulation and configurable decay shape
- 4-stage Schroeder allpass diffusion network with variable density
- Frequency-dependent decay via per-line crossover filters (LP or high-shelf mode)
- Chorus and stereo widening post-processing
- Gain staging — input gain and output trim
- Saturation — drive, mix, and tone shaping
- Timbre filter — XY-controlled cutoff/resonance with pre-drive
- Vibrato — rate, depth, and fade-in modulation
- Pitch Shifter — continuous or octave-snap frequency shift (110–1760 Hz)
- Hand gesture control — webcam + MediaPipe HandLandmarker maps gestures to plugin parameters in real time
- 7 factory presets
- Neumorphic UI embedded via JUCE WebBrowserComponent
- VST3, AU, and Standalone formats

### Technical
- Built with JUCE 8.0.4 + C++17 + CMake 3.22+
- Real-time safe — no heap allocations in processBlock
- Svelte 5 + TypeScript + Tailwind CSS frontend
- Self-contained binary — no external web server or internet required at runtime

### System Requirements
- macOS 11.0 or later
- Intel or Apple Silicon
- Built-in or external webcam (required for gesture control)

### Known Limitations
- macOS only (Windows planned)
- Webcam required for gesture features; plugin works without it but gesture control will be inactive
- Y-Track gesture mode is in development — parameters are wired but gesture mapping is incomplete

### Download
[Download PosTalk v0.3.0](https://github.com/IAndy-10/PosTalk/releases/tag/v0.3.0)
