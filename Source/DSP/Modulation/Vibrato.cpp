#include "Vibrato.h"

void Vibrato::prepare(double sampleRate) {
    sr = sampleRate;

    const int bufSize = (int)(kMaxDepthMs * 2.0f * (float)sampleRate / 1000.0f) + 4;
    for (auto& ch : delayBuf)
        ch.assign(bufSize, 0.0f);

    writeIdx           = 0;
    phase[0]           = 0.0f;
    phase[1]           = 0.25f;  // R channel 90° behind L — decorrelates pitch shift
    fadeGain           = 0.0f;
    fadeSamplesLeft    = 0;
    fadeInTotalSamples = 1;
    prevDepth          = 0.0f;

    auto init = [&](juce::SmoothedValue<float>& sv, float v) {
        sv.reset(sampleRate, 0.01);
        sv.setCurrentAndTargetValue(v);
    };
    init(smoothRate,   2.0f);
    init(smoothDepth,  0.0f);
    init(smoothFadeIn, 100.0f);
}

void Vibrato::reset() {
    for (auto& ch : delayBuf) std::fill(ch.begin(), ch.end(), 0.0f);
    writeIdx           = 0;
    phase[0]           = 0.0f;
    phase[1]           = 0.25f;
    fadeGain           = 0.0f;
    fadeSamplesLeft    = 0;
    fadeInTotalSamples = 1;
    prevDepth          = 0.0f;
}

void Vibrato::setRate(float hz)     { smoothRate.setTargetValue(hz); }
void Vibrato::setDepth(float v)     { smoothDepth.setTargetValue(v); }
void Vibrato::setFadeInMs(float ms) { smoothFadeIn.setTargetValue(ms); }

void Vibrato::process(juce::AudioBuffer<float>& buffer) {
    const int   numChannels = buffer.getNumChannels();
    const int   numSamples  = buffer.getNumSamples();
    const int   bufSize     = (int)delayBuf[0].size();
    const float baseDelay   = kMaxDepthMs * (float)sr / 1000.0f;
    const float twoPi       = juce::MathConstants<float>::twoPi;

    for (int s = 0; s < numSamples; ++s) {
        const float rate     = smoothRate.getNextValue();
        const float depth    = smoothDepth.getNextValue();
        const float fadeInMs = smoothFadeIn.getNextValue();

        // Reset fade-in ramp when depth transitions from zero to positive
        if (prevDepth <= 0.0f && depth > 0.0f) {
            fadeInTotalSamples = juce::jmax(1, (int)(fadeInMs * (float)sr / 1000.0f));
            fadeSamplesLeft    = fadeInTotalSamples;
            fadeGain           = 0.0f;
        }
        prevDepth = depth;

        // Advance linear fade ramp
        if (fadeSamplesLeft > 0) {
            fadeGain = 1.0f - (float)fadeSamplesLeft / (float)fadeInTotalSamples;
            --fadeSamplesLeft;
        } else {
            fadeGain = (depth > 0.0f) ? 1.0f : 0.0f;
        }

        const float effectiveDepth = depth * fadeGain;

        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
            // Per-channel LFO: L at phase[0], R at phase[1] (quarter-cycle apart)
            // This decorrelates the pitch modulation between channels.
            phase[ch] += rate / (float)sr;
            if (phase[ch] >= 1.0f) phase[ch] -= 1.0f;
            const float lfoOut = std::sin(phase[ch] * twoPi);

            const float delaySamples = baseDelay + lfoOut * effectiveDepth * baseDelay;

            // Write current sample into circular buffer
            delayBuf[ch][writeIdx] = buffer.getWritePointer(ch)[s];

            // Fractional read with linear interpolation
            float readPos = (float)writeIdx - delaySamples;
            if (readPos < 0.0f) readPos += (float)bufSize;

            const float floorPos = std::floor(readPos);
            const float frac     = readPos - floorPos;
            const int   i0       = (int)floorPos % bufSize;
            const int   i1       = (i0 + 1) % bufSize;

            buffer.getWritePointer(ch)[s] = delayBuf[ch][i0] * (1.0f - frac)
                                          + delayBuf[ch][i1] * frac;
        }

        writeIdx = (writeIdx + 1) % bufSize;
    }
}
