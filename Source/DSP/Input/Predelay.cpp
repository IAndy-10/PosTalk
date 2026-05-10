#include "Predelay.h"
#include <algorithm>

static constexpr double kMaxDelayMs = 500.0;

void Predelay::prepare(double sampleRate) {
    sr = sampleRate;
    int maxSamples = static_cast<int>(sampleRate * kMaxDelayMs * 0.001) + 4;
    delayL.prepare(maxSamples);
    delayR.prepare(maxSamples);

    float initialSamples = static_cast<float>(sr * static_cast<double>(delayMs) * 0.001);
    smoothDelay.reset(sampleRate, 0.02);  // 20 ms ramp for automation smoothness
    smoothDelay.setCurrentAndTargetValue(juce::jmax(0.0f, initialSamples));
}

void Predelay::setDelayMs(float ms) {
    delayMs = ms;
    float newSamples = static_cast<float>(sr * static_cast<double>(ms) * 0.001);
    smoothDelay.setTargetValue(juce::jmax(0.0f, newSamples));
}

void Predelay::reset() {
    delayL.clear();
    delayR.clear();
}

void Predelay::process(juce::AudioBuffer<float>& buffer) {
    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        const float d = smoothDelay.getNextValue();

        if (numChannels >= 1) {
            float* data    = buffer.getWritePointer(0);
            // Read before write to avoid feeding back the sample just written.
            float  delayed = (d > 0.0f) ? delayL.readInterpolated(d) : data[i];
            delayL.write(data[i]);
            data[i] = delayed;
        }

        if (numChannels >= 2) {
            float* data    = buffer.getWritePointer(1);
            float  delayed = (d > 0.0f) ? delayR.readInterpolated(d) : data[i];
            delayR.write(data[i]);
            data[i] = delayed;
        }
    }
}
