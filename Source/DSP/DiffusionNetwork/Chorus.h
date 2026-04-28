#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "DiffusionNetwork/DelayLine.h"
#include "DiffusionNetwork/LFO.h"

// Stereo chorus effect using LFO-modulated delay lines.
// Adds subtle movement to the reverb tail.
class Chorus {
public:
    void prepare(double sampleRate);
    void setAmount(float amount); // 0-1: modulation depth
    void setRate(float hz);       // LFO frequency

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr float BASE_DELAY_MS = 12.0f; // center delay
    static constexpr float MAX_DEPTH_MS  = 6.0f;  // max modulation depth

    double sr = 44100.0;
    float  amount = 0.2f;
    float  rate   = 1.5f;

    DelayLine delayL, delayR;
    LFO       lfoL, lfoR;

    int baseDelaySamples = 0;
    float depthSamples   = 0.0f;
};

