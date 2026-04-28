# AudioPlugin02 — Claude Instructions

## Project
JUCE audio plugin (VST3 / AU / Standalone) with a Svelte WebView UI.
Reverb effect with FDN architecture.

## Build
```bash
cd build
cmake .. && make -j$(sysctl -n hw.ncpu)
```
Only re-run `cmake ..` when `CMakeLists.txt` changes. Otherwise just `make -j$(sysctl -n hw.ncpu)`.

## Frontend
Svelte UI lives in `WebUI/`. Built automatically by `make`, but can be built manually:
```bash
cd WebUI && npm run build
```
If the build fails with a Vite module error, delete `node_modules` and reinstall:
```bash
rm -rf node_modules package-lock.json && npm install
```

## Testing
- **Standalone app:** `open "build/AudioPlugin02_artefacts/Standalone/Audio Plugin 02.app"`
- **VST3** is auto-copied to `~/Library/Audio/Plug-Ins/VST3/` after each build (COPY_PLUGIN_AFTER_BUILD TRUE)
- Load in Ableton or any DAW from that path (rescan plugins if needed)

## Key files
- `Source/Plugin/PluginProcessor.cpp` — audio processing
- `Source/Plugin/PluginEditor.cpp` — WebView UI host
- `Source/Plugin/WebviewBridge.h` — JS <-> JUCE bridge
- `Source/Parameters/ParameterIDs.h` — parameter definitions
- `Source/Parameters/ParameterLayout.h` — APVTS layout
- `WebUI/src/App.svelte` — main UI component
- `WebUI/src/bridge/bridge.ts` — frontend bridge
- `WebUI/src/state/store.ts` — Svelte state store

## Branches
- `main` — stable
- `audio-player-reverb-01` — current development branch
