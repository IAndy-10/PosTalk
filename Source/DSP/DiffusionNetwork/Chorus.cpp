#include "Chorus.h"
#include <cmath>
#include <algorithm>

void Chorus::prepare(double sampleRate) {
    sr = sampleRate;

    int maxSamples = static_cast<int>(sampleRate * 0.05) + 4; // 50 ms max
    delayL.prepare(maxSamples);
    delayR.prepare(maxSamples);

    baseDelaySamples = static_cast<int>(sampleRate * BASE_DELAY_MS * 0.001);

    lfoL.prepare(sampleRate);
    lfoR.prepare(sampleRate);
    lfoL.setFrequency(rate);
    lfoR.setFrequency(rate);

    // Offset R LFO by 90 degrees for stereo width
    for (int i = 0; i < static_cast<int>(sampleRate / 4.0 / rate); ++i)
        lfoR.getNext();

    smoothAmount.reset(sampleRate, 0.02);   // 20 ms smoothing on depth
    smoothAmount.setCurrentAndTargetValue(0.0f);

    enableRamp.reset(sampleRate, 0.015);    // 15 ms fade-in/out for bypass
    enableRamp.setCurrentAndTargetValue(0.0f);
}

void Chorus::setEnabled(bool on) {
    enableRamp.setTargetValue(on ? 1.0f : 0.0f);
}

void Chorus::setAmount(float a) {
    smoothAmount.setTargetValue(juce::jlimit(0.0f, 1.0f, a));
}

void Chorus::setRate(float hz) {
    rate = hz;
    lfoL.setFrequency(hz);
    lfoR.setFrequency(hz);
}

void Chorus::reset() {
    delayL.clear();
    delayR.clear();
    lfoL.reset();
    lfoR.reset();
}

void Chorus::process(juce::AudioBuffer<float>& buffer) {
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    int numSamples  = buffer.getNumSamples();

    const float maxDepthSamples = static_cast<float>(sr * MAX_DEPTH_MS * 0.001);

    for (int i = 0; i < numSamples; ++i) {
        const float effectiveAmount = smoothAmount.getNextValue() * enableRamp.getNextValue();

        if (effectiveAmount < 1e-5f) {
            // Keep delay buffers and LFOs running so re-enable is seamless.
            lfoL.getNext();
            lfoR.getNext();
            if (numChannels >= 1) delayL.write(buffer.getSample(0, i));
            if (numChannels >= 2) delayR.write(buffer.getSample(1, i));
            continue;
        }

        float modL = lfoL.getNext() * maxDepthSamples * effectiveAmount;
        float modR = lfoR.getNext() * maxDepthSamples * effectiveAmount;

        float readPosL = static_cast<float>(baseDelaySamples) + modL;
        float readPosR = static_cast<float>(baseDelaySamples) + modR;
        readPosL = std::max(1.0f, readPosL);
        readPosR = std::max(1.0f, readPosR);

        if (numChannels >= 1) {
            float inL = buffer.getSample(0, i);
            delayL.write(inL);
            float wet = delayL.readInterpolated(readPosL);
            buffer.setSample(0, i, inL + effectiveAmount * (wet - inL));
        }

        if (numChannels >= 2) {
            float inR = buffer.getSample(1, i);
            delayR.write(inR);
            float wet = delayR.readInterpolated(readPosR);
            buffer.setSample(1, i, inR + effectiveAmount * (wet - inR));
        }
    }
}
