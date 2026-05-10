#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Mid-Side stereo widener.
// - Low-frequency protection on the side channel (~80 Hz HP) preserves mono compatibility.
// - Width is smoothed to prevent automation clicks.
// - Width is clamped to [0, 2]: 1.0 = original stereo, 0.0 = mono, 2.0 = doubled width.
class StereoWidener {
public:
    void prepare(double sampleRate) {
        smoothWidth.reset(sampleRate, 0.01);
        smoothWidth.setCurrentAndTargetValue(width);
        // One-pole LP coefficient for ~80 Hz (used to HP the side channel)
        kLF = static_cast<float>(
            1.0 - std::exp(-juce::MathConstants<double>::twoPi * 80.0 / sampleRate));
        lpSideState = 0.0f;
    }

    void setWidth(float w) {
        width = juce::jlimit(0.0f, 2.0f, w);
        smoothWidth.setTargetValue(width);
    }

    void process(juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2) return;

        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i) {
            const float w    = smoothWidth.getNextValue();
            const float mid  = (L[i] + R[i]) * 0.5f;
            const float side = (L[i] - R[i]) * 0.5f;

            // Low-frequency protection: one-pole LP on the side signal, then subtract
            // to get an HP-filtered side. Keeps bass centred at any width setting.
            lpSideState += kLF * (side - lpSideState);
            const float sideProt = side - lpSideState;

            L[i] = mid + sideProt * w;
            R[i] = mid - sideProt * w;
        }
    }

private:
    float width       = 1.0f;
    float lpSideState = 0.0f;
    float kLF         = 0.01f;  // set properly in prepare()

    juce::SmoothedValue<float> smoothWidth { 1.0f };
};
