#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

class Saturation {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        auto init = [&](juce::SmoothedValue<float>& sv, float v) {
            sv.reset(sampleRate, 0.01);
            sv.setCurrentAndTargetValue(v);
        };
        init(smoothDrive, 0.0f);
        init(smoothMix,   0.0f);
        init(smoothTone,  1.0f);
        aaState.fill(0.0f);
        toneState.fill(0.0f);
        prevSample.fill(0.0f);
    }

    void setDrive(float v)  { smoothDrive.setTargetValue(v); }           // 0–1
    void setMix(float pct)  { smoothMix.setTargetValue(pct / 100.0f); }  // 0–100 → 0–1
    void setTone(float v)   { smoothTone.setTargetValue(v); }            // 0–1

    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples  = buffer.getNumSamples();

        // Tone coefficient: per block is fine (filter cutoff, not direct amplitude)
        const float tone  = smoothTone.skip(numSamples);
        const float fc    = 500.0f + 19500.0f * tone;
        const float kTone = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * fc / sr);
        constexpr float kAA = 0.3f;

        // Outer loop: samples — so smoothDrive / smoothMix advance once per sample
        // and both channels share the same smoothed value (matches GainModule pattern).
        for (int s = 0; s < numSamples; ++s) {
            const float drv = smoothDrive.getNextValue();
            const float mix = smoothMix.getNextValue();

            // Waveshaper drive: 0 → identity (d≈0.001), 1 → heavy clip (d≈10)
            const float d     = 0.001f + drv * 9.999f;
            const float tanhD = std::tanh(d);

            for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
                const float dry = buffer.getWritePointer(ch)[s];

                // 2× virtual oversampling:
                //   upsample via linear interp → shape both sub-samples → average downsample
                const float up0 = 0.5f * (prevSample[ch] + dry);
                const float w0  = std::tanh(up0 * d) / tanhD;
                const float w1  = std::tanh(dry  * d) / tanhD;
                float       wet = 0.5f * (w0 + w1);

                // Anti-aliasing one-pole LP (k≈0.3)
                aaState[ch] += kAA * (wet - aaState[ch]);

                // Post-saturation tone LP
                toneState[ch] += kTone * (aaState[ch] - toneState[ch]);

                // Wet/dry mix
                buffer.getWritePointer(ch)[s] = dry + mix * (toneState[ch] - dry);
                prevSample[ch] = dry;
            }
        }
    }

private:
    float sr = 44100.0f;

    juce::SmoothedValue<float> smoothDrive { 0.0f };
    juce::SmoothedValue<float> smoothMix   { 0.0f };
    juce::SmoothedValue<float> smoothTone  { 1.0f };

    std::array<float, 2> aaState    {};
    std::array<float, 2> toneState  {};
    std::array<float, 2> prevSample {};
};
