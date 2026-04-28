#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// Mid-Side stereo widener.
// width = 1.0  → original stereo (no processing)
// width = 0.0  → mono (side = 0)
// width = 2.0  → doubled stereo width
class StereoWidener {
public:
    void setWidth(float w) { width = w; }

    void process(juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2) return;

        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i) {
            float mid  = (L[i] + R[i]) * 0.5f;
            float side = (L[i] - R[i]) * 0.5f * width;
            L[i] = mid + side;
            R[i] = mid - side;
        }
    }

private:
    float width = 1.0f;
};

