#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

class Saturation {
public:
    void prepare(double sampleRate);

    void setDrive(float v);          // 0–1
    void setMix(float pct);          // 0–100 → 0–1
    void setTone(float v);           // 0–1

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    float sr = 44100.0f;

    juce::SmoothedValue<float> smoothDrive { 0.0f };
    juce::SmoothedValue<float> smoothMix   { 0.0f };
    juce::SmoothedValue<float> smoothTone  { 1.0f };

    std::array<float, 2> aaState    {};
    std::array<float, 2> toneState  {};
    std::array<float, 2> prevSample {};
};
