#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class GainModule {
public:
    void prepare(double sampleRate) {
        auto init = [&](juce::SmoothedValue<float>& sv, float v) {
            sv.reset(sampleRate, 0.01);
            sv.setCurrentAndTargetValue(v);
        };
        init(smoothGain, 1.0f);
        init(smoothTrim, 1.0f);
    }

    // gainDb: −24 to +12 dB
    void setGainDb(float dB) {
        smoothGain.setTargetValue(juce::Decibels::decibelsToGain(dB));
    }

    // outputTrimDb: −12 to 0 dB
    void setOutputTrimDb(float dB) {
        smoothTrim.setTargetValue(juce::Decibels::decibelsToGain(dB));
    }

    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();
        for (int s = 0; s < numSamples; ++s) {
            float g = smoothGain.getNextValue() * smoothTrim.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer(ch)[s] *= g;
        }
    }

private:
    juce::SmoothedValue<float> smoothGain { 1.0f };
    juce::SmoothedValue<float> smoothTrim { 1.0f };
};
