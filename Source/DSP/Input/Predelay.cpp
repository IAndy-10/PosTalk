#include "Predelay.h"
#include <algorithm>

static constexpr double kMaxDelayMs = 500.0;

void Predelay::prepare(double sampleRate) {
    sr = sampleRate;
    int maxSamples = static_cast<int>(sampleRate * kMaxDelayMs * 0.001) + 4;
    delayL.prepare(maxSamples);
    delayR.prepare(maxSamples);
    setDelayMs(delayMs);
}

void Predelay::setDelayMs(float ms) {
    delayMs = ms;
    delaySamples = static_cast<int>(sr * static_cast<double>(ms) * 0.001);
    delaySamples = std::max(0, delaySamples);
}

void Predelay::reset() {
    delayL.clear();
    delayR.clear();
}

void Predelay::process(juce::AudioBuffer<float>& buffer) {
    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();

    if (delaySamples <= 0) return;

    if (numChannels >= 1) {
        float* data = buffer.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i) {
            float delayed = delayL.read(delaySamples);
            delayL.write(data[i]);
            data[i] = delayed;
        }
    }

    if (numChannels >= 2) {
        float* data = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i) {
            float delayed = delayR.read(delaySamples);
            delayR.write(data[i]);
            data[i] = delayed;
        }
    }
}

