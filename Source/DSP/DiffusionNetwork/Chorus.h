#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "DelayLine.h"
#include "LFO.h"

// Stereo chorus effect using LFO-modulated delay lines.
// Adds subtle movement to the reverb tail.
// Amount is smoothed and bypass uses a fade ramp to prevent clicks.
class Chorus {
public:
    void prepare(double sampleRate);
    void setEnabled(bool on);     // click-free bypass via fade ramp
    void setAmount(float amount); // 0-1: modulation depth (smoothed)
    void setRate(float hz);       // LFO frequency

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr float BASE_DELAY_MS = 12.0f;
    static constexpr float MAX_DEPTH_MS  = 6.0f;

    double sr   = 44100.0;
    float  rate = 1.5f;

    DelayLine delayL, delayR;
    LFO       lfoL, lfoR;

    int baseDelaySamples = 0;

    juce::SmoothedValue<float> smoothAmount { 0.0f };  // modulation depth — smoothed
    juce::SmoothedValue<float> enableRamp   { 0.0f };  // 0=off → 1=on, click-free bypass
};
