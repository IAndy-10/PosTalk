#pragma once
#include "ParameterIDs.h"

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    using namespace PluginParamIDs;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ParameterLayout (grouped by WebUI panels in `WebUI/src/App.svelte`)

    // ===== PANEL 1: INPUT =====
    // Input filter
    params.push_back(std::make_unique<juce::AudioParameterBool>(loCutEnabled, "Lo Cut", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(hiCutEnabled, "Hi Cut", false));
    // Both filters share the same 50–18000 Hz range. The UI enforces lo < hi ordering.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        loCutFreq, "Lo Cut Freq",
        juce::NormalisableRange<float>(50.0f, 18000.0f, 1.0f, 0.3f), 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        hiCutFreq, "Hi Cut Freq",
        juce::NormalisableRange<float>(50.0f, 18000.0f, 1.0f, 0.3f), 8000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        hiCutQ, "Hi Cut Q",
        juce::NormalisableRange<float>(0.5f, 9.0f, 0.01f, 0.5f), 0.7071f));
    // new_parameter
    //params.push_back(std::make_unique<juce::AudioParameterFloat>(
      //      hiCutQ, "Lo Cut Q",
        //    juce::NormalisableRange<float>(0.5f, 9.0f, 0.01f, 0.5f), 0.7071f));

    // Predelay subsection
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        predelay, "Predelay",
        juce::NormalisableRange<float>(0.0f, 500.0f, 0.1f, 0.5f), 20.0f));

    // ===== PANEL 2: EARLY REFLECTIONS =====
    params.push_back(std::make_unique<juce::AudioParameterBool>(erEnabled, "ER Spin", false));
    // erAmount stored in display units (2–55). PluginProcessor normalises to 0–1 before DSP.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        erAmount, "ER Amount",
        juce::NormalisableRange<float>(2.0f, 55.0f, 0.01f, 0.5f), 10.0f));
    // erRate stored in Hz (0.07–1.3). Passed directly to SpinModulator.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        erRate, "ER Rate",
        juce::NormalisableRange<float>(0.07f, 1.3f, 0.01f, 0.5f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        erShape, "ER Shape",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // Reflect gain live in the audio path but are controlled in this panel:
    // - reflectGain: Early Reflections panel
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        reflectGain, "Reflect Gain",
        juce::NormalisableRange<float>(-30.0f, 6.0f, 0.1f), 0.0f));

    // ===== PANEL 3: DIFFUSION NETWORK =====
    // FDN / diffusion network core
    params.push_back(std::make_unique<juce::AudioParameterInt>(reverbMode, "Mode", 0, 1, 0));
    // highFilterType: false = one-pole LP (original), true = first-order high shelf.
    // When shelving mode is active, the damping parameter has no effect at the DSP level.
    params.push_back(std::make_unique<juce::AudioParameterBool>(highFilterType, "High Filter Type", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        crossoverFreq, "Crossover Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.5f), 3000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        diffusion, "Diffusion",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        scale, "Scale",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // scale (0-1): blends between raw and fully-diffused FDN input. 0=no diffusion, 1=full diffusion.
    // Note: damping/feedback are part of the Diffusion Network panel UI (and graph).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        damping, "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        feedback, "Feedback",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f));

    // Chorus subsection (Diffusion Network panel)
    params.push_back(std::make_unique<juce::AudioParameterBool>(chorusEnabled, "Chorus Enable", false));
    // chorusAmount range corrected from 0–1 to 0.01–4.0 (Step 02 preliminary fix).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        chorusAmount, "Chorus Amount",
        juce::NormalisableRange<float>(0.01f, 4.0f, 0.01f), 0.2f));
    // chorusRate range corrected from 0.1–10 Hz to 0.01–8.0 Hz (Step 02 preliminary fix).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        chorusRate, "Chorus Rate",
        juce::NormalisableRange<float>(0.01f, 8.0f, 0.01f, 0.5f), 1.5f));

    // Diffuse gains live in the audio path but is controlled in this panel:
    // - diffuseGain: Diffusion Network panel
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        diffuseGain, "Diffuse Gain",
        juce::NormalisableRange<float>(-30.0f, 6.0f, 0.1f), 0.0f));

    // ===== PANEL 4: DECAY =====
    // Decay range extended to 60 s (60000 ms). RT60 formula remains stable at max value.
    // At RT60=60s and longest FDN line (~23ms): g = 10^(-3*0.023/60) ≈ 0.997 < 0.9999. ✓
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        decay, "Decay",
        juce::NormalisableRange<float>(200.0f, 60000.0f, 1.0f, 0.3f), 1500.0f));

    // ===== PANEL 5: OUTPUT =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        dryWet, "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));



    // ===== PANEL 3: DIFFUSION NETWORK =====
    // Size subsection + auxiliary selectors
    params.push_back(std::make_unique<juce::AudioParameterInt>(smooth, "Smooth", 0, 3, 0));
    // size (0-1, normalized): controls FDN delay length scaling in PluginProcessor via fdnReverb.setSize().
    // Display mapping (0-1 → 0.22-500) is handled in the UI layer.
    // Note: 'scale' parameter also exists in APVTS but is currently unconnected to any DSP module.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        size, "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // ===== PANEL 4: DECAY =====
    params.push_back(std::make_unique<juce::AudioParameterBool>(freeze, "Freeze", false));
    // flatEnabled / cutEnabled replace the old flatCut Int (0-1). DSP wiring TBD.
    params.push_back(std::make_unique<juce::AudioParameterBool>(flatEnabled, "Flat", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(cutEnabled,  "Cut",  false));

    // ===== PANEL 5: OUTPUT =====
    // stereo stored in degrees (0-120). PluginProcessor converts: width = degrees / 120.
    // 0° = mono (width 0.0), 120° = unchanged stereo (width 1.0).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        stereo, "Stereo",
        juce::NormalisableRange<float>(0.0f, 120.0f, 0.1f), 120.0f));
    // Density option labels: 0=Sparse, 1=Low, 2=Mid, 3=High.
    // Default 3 = High = 4 active diffusion stages (matches the DSP default behaviour).
    params.push_back(std::make_unique<juce::AudioParameterInt>(density, "Density", 0, 3, 3));

    // ===== MODULE 1: GAIN =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        gainDb, "Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        outputTrimDb, "Output Trim",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), 0.0f));

    // ===== MODULE 2: TIMBRE =====
    // Cutoff: logarithmic feel via skew 0.3
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        timbreCutoff, "Timbre Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 8000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        timbreResonance, "Timbre Resonance",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.01f, 0.5f), 0.7071f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        timbreDrivePre, "Timbre Pre-Drive",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 0.0f));

    // ===== MODULE 3: SATURATION =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        satDrive, "Sat Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        satMix, "Sat Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        satTone, "Sat Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    // ===== MODULE 5: MODULATION =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        vibratoRate, "Vibrato Rate",
        juce::NormalisableRange<float>(0.1f, 8.0f, 0.01f, 0.5f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        vibratoDepth, "Vibrato Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        vibratoFadeIn, "Vibrato Fade In",
        juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f, 0.5f), 100.0f));

    // ===== MODULE 6: PITCH SHIFTER =====
    // 110–1760 Hz covers ±2 octaves around 440 Hz (unity = no shift).
    // Logarithmic skew (0.5) gives fine resolution near unity.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        pitchFrequency, "Pitch Frequency",
        juce::NormalisableRange<float>(110.0f, 1760.0f, 0.1f, 2.0f), 440.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        pitchOctaveStep, "Pitch Octave Step", false));

    // ===== PERFORMANCE CONTROLS =====
    params.push_back(std::make_unique<juce::AudioParameterBool>(sustainEnabled, "Sustain", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(cutNow, "Cut Now", false));

    // ===== STAGE 2: MACRO Y =====
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        macroY, "Macro Y",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // ===== Y AMOUNT (modulation depth per parameter, range -1 to +1) =====
    // Default 0 = no gesture effect. Positive = Y increases param, negative = Y decreases.
    auto makeYAmt = [&](const juce::ParameterID& pid, const juce::String& name) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            pid, name, juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
    };
    makeYAmt(gainYAmt,         "Gain Y Amt");
    makeYAmt(cutoffYAmt,       "Cutoff Y Amt");
    makeYAmt(satDriveYAmt,     "Sat Drive Y Amt");
    makeYAmt(satMixYAmt,       "Sat Mix Y Amt");
    makeYAmt(vibratoDepthYAmt, "Vibrato Depth Y Amt");
    makeYAmt(dryWetYAmt,       "DryWet Y Amt");
    makeYAmt(decayYAmt,        "Decay Y Amt");
    makeYAmt(dampingYAmt,      "Damping Y Amt");

    // ===== GESTURE MODE TOGGLE =====
    // false = classic up/down → dryWet, true = index-tip Y → macroY
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        gestureModeToggle, "Gesture Mode", false));

    return { params.begin(), params.end() };
}