#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include "DiffusionNetwork/DelayLine.h"
#include "EarlyReflections/SpinModulator.h"

// Multi-tap early reflections with spin modulation.
// 8 taps for L and 8 for R with prime delay lengths.
class EarlyReflections {
public:
    void prepare(double sampleRate);
    void setEnabled(bool on);
    void setAmount(float amount); // 0-1
    void setRate(float hz);       // Spin LFO rate
    void setShape(float shape);   // 0-1: blend between tap distribution profiles

    // Additive: mixes ER output into buffer (original behaviour)
    void process(juce::AudioBuffer<float>& buffer);

    // Separate: reads input as FDN feed, writes ER-only contribution to output.
    // Use this when reflectGain and diffuseGain must be applied independently.
    void processOut(const juce::AudioBuffer<float>& input,
                    juce::AudioBuffer<float>& output);

    void reset();

private:
    static constexpr int NUM_TAPS = 8;

    // Prime tap delays in samples at 44100 Hz (scaled for other rates)
    static constexpr int BASE_TAPS_L[NUM_TAPS] = { 89, 151, 211, 347, 431, 557, 683, 809 };
    static constexpr int BASE_TAPS_R[NUM_TAPS] = { 107, 167, 241, 367, 461, 587, 709, 839 };

    // Gradual (shape=0): slow exponential decay across taps.
    static constexpr float TAP_GAINS_GRADUAL[NUM_TAPS] = {
        0.85f, 0.70f, 0.60f, 0.50f, 0.40f, 0.35f, 0.28f, 0.22f
    };
    // Rapid (shape=1): steep early cut-off, energy concentrated in first taps.
    static constexpr float TAP_GAINS_RAPID[NUM_TAPS] = {
        0.90f, 0.55f, 0.33f, 0.20f, 0.12f, 0.07f, 0.04f, 0.02f
    };

    std::array<DelayLine, NUM_TAPS> delayL;
    std::array<DelayLine, NUM_TAPS> delayR;

    SpinModulator spin;

    double sr       = 44100.0;
    bool   enabled  = false;
    float  amount   = 0.3f;
    float  shape    = 0.5f;
    std::array<int, NUM_TAPS> tapL {}, tapR {};
};

