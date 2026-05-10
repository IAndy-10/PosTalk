#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

class TimbreFilter {
public:
    void prepare(double sampleRate);

    void setCutoff(float hz);
    void setResonance(float q);
    void setDrivePreDb(float dB);

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    double sr = 44100.0;

    juce::SmoothedValue<float> smoothCutoff    { 8000.0f };
    juce::SmoothedValue<float> smoothResonance { 0.7071f };
    juce::SmoothedValue<float> smoothDrivePre  { 1.0f };

    std::array<std::array<double, 2>, 2> state {};  // [channel][DF2T state 0/1]
};
