#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <cmath>

class Vibrato {
public:
    // Maximum modulation swing (½ period of the delay excursion)
    static constexpr float kMaxDepthMs = 5.0f;

    void prepare(double sampleRate);

    // vibratoRate in Hz, vibratoDepth 0-1, vibratoFadeIn in ms (raw APVTS values)
    void setRate(float hz);
    void setDepth(float v);
    void setFadeInMs(float ms);

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    double sr = 44100.0;

    std::array<std::vector<float>, 2> delayBuf;
    int   writeIdx = 0;
    float phase[2] = { 0.0f, 0.25f };  // [0]=L at 0°, [1]=R at 90° — decorrelated

    // FadeIn state
    float fadeGain           = 0.0f;
    int   fadeSamplesLeft    = 0;
    int   fadeInTotalSamples = 1;
    float prevDepth          = 0.0f;

    juce::SmoothedValue<float> smoothRate   { 2.0f   };
    juce::SmoothedValue<float> smoothDepth  { 0.0f   };
    juce::SmoothedValue<float> smoothFadeIn { 100.0f };
};
