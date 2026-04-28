// ParameterIDs.h
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace PluginParamIDs {
    // Input Filter
    const juce::ParameterID loCutEnabled { "loCutEnabled", 1 };
    const juce::ParameterID hiCutEnabled { "hiCutEnabled", 1 };
    const juce::ParameterID loCutFreq    { "loCutFreq",    1 };
    const juce::ParameterID hiCutFreq    { "hiCutFreq",    1 };
    const juce::ParameterID hiCutQ       { "hiCutQ",       1 };

    // Early Reflections
    const juce::ParameterID erEnabled { "erEnabled", 1 };
    const juce::ParameterID erAmount  { "erAmount",  1 };
    const juce::ParameterID erRate    { "erRate",    1 };
    const juce::ParameterID erShape   { "erShape",   1 };

    // FDN Reverb Core
    const juce::ParameterID reverbMode    { "reverbMode",    1 };
    const juce::ParameterID crossoverFreq { "crossoverFreq", 1 };
    const juce::ParameterID diffusion     { "diffusion",     1 };
    const juce::ParameterID scale         { "scale",         1 };
    const juce::ParameterID decay         { "decay",         1 };
    const juce::ParameterID damping       { "damping",       1 };
    const juce::ParameterID feedback      { "feedback",      1 };

    // Chorus
    const juce::ParameterID chorusEnabled { "chorusEnabled", 1 };
    const juce::ParameterID chorusAmount { "chorusAmount", 1 };
    const juce::ParameterID chorusRate   { "chorusRate",   1 };

    // Output / Reflect
    const juce::ParameterID reflectGain  { "reflectGain",  1 };
    const juce::ParameterID diffuseGain  { "diffuseGain",  1 };
    const juce::ParameterID dryWet       { "dryWet",       1 };

    // Bottom Utility Row / Decay Section
    const juce::ParameterID predelay    { "predelay",    1 };
    const juce::ParameterID smooth      { "smooth",      1 };
    const juce::ParameterID size        { "size",        1 };
    const juce::ParameterID freeze      { "freeze",      1 };
    const juce::ParameterID flatEnabled { "flatEnabled", 1 }; // replaces flatCut (Int 0-1)
    const juce::ParameterID cutEnabled  { "cutEnabled",  1 }; // replaces flatCut (Int 0-1)
    const juce::ParameterID stereo      { "stereo",      1 };
    const juce::ParameterID density     { "density",     1 };

    // FDN high-frequency filter mode
    // false = one-pole low-pass (original), true = first-order high shelf
    const juce::ParameterID highFilterType { "highFilterType", 1 };

    // ===== MODULE 1: GAIN =====
    const juce::ParameterID gainDb       { "gainDb",       1 };
    const juce::ParameterID outputTrimDb { "outputTrimDb", 1 };

    // ===== MODULE 2: TIMBRE =====
    const juce::ParameterID timbreCutoff    { "timbreCutoff",    1 };
    const juce::ParameterID timbreResonance { "timbreResonance", 1 };
    const juce::ParameterID timbreDrivePre  { "timbreDrivePre",  1 };

    // ===== MODULE 3: SATURATION =====
    const juce::ParameterID satDrive { "satDrive", 1 };
    const juce::ParameterID satMix   { "satMix",   1 };
    const juce::ParameterID satTone  { "satTone",  1 };

    // ===== MODULE 5: MODULATION =====
    const juce::ParameterID vibratoRate   { "vibratoRate",   1 };
    const juce::ParameterID vibratoDepth  { "vibratoDepth",  1 };
    const juce::ParameterID vibratoFadeIn { "vibratoFadeIn", 1 };

    // ===== STAGE 2: MACRO Y =====
    const juce::ParameterID macroY { "macroY", 1 };

    // ===== Y AMOUNT (modulation depth per parameter) =====
    // Range -1 to +1. At 0 the gesture has no effect on that parameter.
    const juce::ParameterID gainYAmt          { "gainYAmt",          1 };
    const juce::ParameterID cutoffYAmt        { "cutoffYAmt",        1 };
    const juce::ParameterID satDriveYAmt      { "satDriveYAmt",      1 };
    const juce::ParameterID satMixYAmt        { "satMixYAmt",        1 };
    const juce::ParameterID vibratoDepthYAmt  { "vibratoDepthYAmt",  1 };
    const juce::ParameterID dryWetYAmt        { "dryWetYAmt",        1 };
    const juce::ParameterID decayYAmt         { "decayYAmt",         1 };
    const juce::ParameterID dampingYAmt       { "dampingYAmt",       1 };

    // ===== GESTURE MODE TOGGLE =====
    // false = classic up/down gesture → dryWet rate control
    // true  = index-tip Y tracking    → macroY continuous
    const juce::ParameterID gestureModeToggle { "gestureModeToggle", 1 };
}