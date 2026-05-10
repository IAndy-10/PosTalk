#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "DiffusionNetwork/DelayLine.h"

// Stereo pre-delay module. Up to 500 ms.
// Delay changes are smoothed per-sample via SmoothedValue + readInterpolated()
// to prevent clicks when the parameter is automated or changed at block rate.
class Predelay {
public:
    void prepare(double sampleRate);
    void setDelayMs(float ms);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    double sr      = 44100.0;
    float  delayMs = 20.0f;

    // Smoothed delay length in samples — ramped over 20 ms on change.
    juce::SmoothedValue<float> smoothDelay { 882.0f };

    DelayLine delayL, delayR;
};
