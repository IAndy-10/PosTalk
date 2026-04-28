#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "DiffusionNetwork/DelayLine.h"

// Stereo pre-delay module. Up to 500 ms.
class Predelay {
public:
    void prepare(double sampleRate);
    void setDelayMs(float ms);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    double sr = 44100.0;
    float  delayMs = 20.0f;
    int    delaySamples = 882; // 20ms at 44100

    DelayLine delayL, delayR;
};

