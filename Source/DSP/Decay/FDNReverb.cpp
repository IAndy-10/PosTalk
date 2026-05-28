#include "FDNReverb.h"
#include <cmath>
#include <algorithm>


void FDNReverb::prepare(double sampleRate) {
    sr = sampleRate;
    double ratio = sampleRate / 44100.0;

    for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
        // Allocate at maximum size (sizeScale=1.0 → factor=1.5) so setSize(1.0) never clamps.
        int maxLen = static_cast<int>(BASE_DELAYS[i] * ratio * 1.5) + 32;
        delayLines[i].prepare(maxLen);
        delayLens[i] = static_cast<int>(BASE_DELAYS[i] * ratio * (0.5 + sizeScale));

        dampFilters[i].prepare(sampleRate);
        dampFilters[i].setCutoffFreq(crossFreq);
        dampFilters[i].setDamping(dampAmount);

        // Stagger modulation LFO frequencies slightly for each line
        float lfoFreq = MOD_FREQ_BASE * (1.0f + static_cast<float>(i) * 0.08f);
        modLFOs[i].prepare(sampleRate);
        modLFOs[i].setFrequency(lfoFreq);

        modDepth[i] = MOD_DEPTH_BASE;

        // Initialise delay-length smoother at current length; ramp 50 ms on size changes.
        smoothDelayLens[i].reset(sampleRate, 0.05);
        smoothDelayLens[i].setCurrentAndTargetValue(static_cast<float>(delayLens[i]));
    }

    diffusion.prepare(sampleRate);
    diffusion.setDiffusion(diffAmount);

    // Freeze crossfade smoother: 0 = normal, 1 = frozen, 50 ms transition.
    freezeBlend.reset(sampleRate, 0.05);
    freezeBlend.setCurrentAndTargetValue(frozen ? 1.0f : 0.0f);

    // Flat crossfade smoother: 0 = filtered, 1 = bypass damping filter, 30 ms transition.
    flatBlend.reset(sampleRate, 0.03);
    flatBlend.setCurrentAndTargetValue(flatEnabledFlag ? 1.0f : 0.0f);

    normalDecayGain.fill(0.0f);
    updateDecayGains();
}

void FDNReverb::setDecayMs(float ms) {
    decayMs = ms;
    updateDecayGains();
}

void FDNReverb::setDamping(float d) {
    dampAmount = d;
    for (auto& f : dampFilters)
        f.setDamping(dampAmount);
}

void FDNReverb::setDiffusion(float d) {
    diffAmount = d;
    diffusion.setDiffusion(d);
}

void FDNReverb::setSize(float s) {
    sizeScale = s;
    if (sr <= 0.0) return;
    double ratio = sr / 44100.0;
    for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
        int newLen = static_cast<int>(BASE_DELAYS[i] * ratio * (0.5 + s));
        newLen = std::min(newLen, delayLines[i].getMaxDelaySamples() - 4);
        delayLens[i] = newLen;
        // Smooth to the new length — avoids the click that would come from an abrupt jump.
        smoothDelayLens[i].setTargetValue(static_cast<float>(newLen));
    }
    updateDecayGains();
}

void FDNReverb::setCrossoverFreq(float hz) {
    crossFreq = hz;
    for (auto& f : dampFilters)
        f.setCutoffFreq(hz);
}

void FDNReverb::setFeedback(float fb) {
    if (fb == feedback) return;
    feedback = fb;
    updateDecayGains();
}

void FDNReverb::setFrozen(bool isFrozen) {
    frozen = isFrozen;
    // Crossfade to/from frozen state over ~20 ms instead of snapping.
    freezeBlend.setTargetValue(isFrozen ? 1.0f : 0.0f);
    updateDecayGains();
}

void FDNReverb::setReverbMode(int m) {
    mode = m;
    float freqScale = (mode == 0) ? 1.5f : 0.6f;
    for (auto& f : dampFilters)
        f.setCutoffFreq(crossFreq * freqScale);
}

void FDNReverb::setHighFilterType(bool shelving) {
    for (auto& f : dampFilters)
        f.setFilterType(shelving);
}

void FDNReverb::setInputScale(float s) {
    inputScale = s;
}

void FDNReverb::setDensity(int d) {
    static constexpr int stageMap[] = { 0, 1, 2, 4 };
    diffusion.setNumStages(stageMap[juce::jlimit(0, 3, d)]);
}

void FDNReverb::setFlatEnabled(bool f) {
    flatEnabledFlag = f;
    flatBlend.setTargetValue(f ? 1.0f : 0.0f);
}
void FDNReverb::setCutEnabled(bool c)  { cutEnabledFlag  = c; }

void FDNReverb::updateDecayGains() {
    // Always compute the RT60-based gain into normalDecayGain.
    // The frozen blend is handled per-sample in process() via freezeBlend.
    float rt60 = decayMs * 0.001f;
    for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
        float delaySec = static_cast<float>(delayLens[i]) / static_cast<float>(sr);
        if (rt60 > 0.0f && delaySec > 0.0f) {
            // g = 10^(-3 * T60 / RT60) → -60 dB after RT60 seconds
            float g = std::pow(10.0f, -3.0f * delaySec / rt60);
            normalDecayGain[i] = g * feedback;
        } else {
            normalDecayGain[i] = 0.0f;
        }
        // Leave headroom from unity to avoid instability at high feedback settings.
        normalDecayGain[i] = std::min(normalDecayGain[i], 0.9998f);
    }
}

void FDNReverb::process(juce::AudioBuffer<float>& buffer) {
    int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    int numSamples  = buffer.getNumSamples();

    if (numChannels < 1) return;

    for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx) {
        // Freeze blend: 0 = normal RT60 decay, 1 = fully frozen (0.9999f per line).
        // Ramps smoothly (50 ms) so toggling freeze does not produce a click.
        const float fb     = freezeBlend.getNextValue();
        // Flat blend: 0 = use damping filter, 1 = bypass (flat = remove HF roll-off).
        // Advanced once per sample; shared across all 8 delay lines this sample.
        // The filter is ALWAYS computed to keep its IIR state valid — we only crossfade
        // the output, so re-engaging the filter never causes a state-mismatch click.
        const float flatB  = flatBlend.getNextValue();

        float inL = buffer.getSample(0, sampleIdx);
        float inR = (numChannels >= 2) ? buffer.getSample(1, sampleIdx) : inL;

        // --- Diffuse input, then blend with raw input (scale param) ---
        float diffL = inL, diffR = inR;
        diffusion.processStereo(diffL, diffR);
        diffL = inL * (1.0f - inputScale) + diffL * inputScale;
        diffR = inR * (1.0f - inputScale) + diffR * inputScale;

        // --- Read delay line outputs and apply damping ---
        std::array<float, N> y;
        for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
            // Per-line modulation (subtle vibrato to reduce metallic ringing)
            float mod = modLFOs[i].getNext() * modDepth[i];

            // Smoothed delay length: ramps toward the target when setSize() is called,
            // preventing the click that a sudden delay-length change would produce.
            float readPos = smoothDelayLens[i].getNextValue() + mod;
            readPos = std::max(1.0f, readPos);

            const float y_raw  = delayLines[i].readInterpolated(readPos);
            const float y_filt = dampFilters[i].processSample(y_raw); // always run — keeps IIR state fresh

            // Flat: crossfade between filtered and raw output.
            // effectiveFlat is 0 when not frozen, so filter is always used outside freeze.
            const float effectiveFlat = flatB * fb;
            y[i] = y_filt * (1.0f - effectiveFlat) + y_raw * effectiveFlat;

            // Blend between normal RT60 decay and frozen sustain (0.9999f).
            const float actualGain = normalDecayGain[i] * (1.0f - fb) + 0.9999f * fb;
            y[i] *= actualGain;
        }

        // --- Apply Hadamard feedback matrix ---
        FeedbackMatrix::apply(y);

        // --- Write back into delay lines (feedback + new input) ---
        // cut mode: when frozen with cut active, block new input to preserve frozen tail
        for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
            float newInput = (i % 2 == 0) ? diffL : diffR;
            float writeVal = (frozen && cutEnabledFlag) ? y[i] : (newInput + y[i]);
            delayLines[i].write(writeVal);
        }

        // --- Mix outputs (even → L, odd → R) ---
        float outL = 0.0f, outR = 0.0f;
        static constexpr float OUT_SCALE = 0.35355339059f; // 1/sqrt(8)
        for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
            if (i % 2 == 0) outL += y[i];
            else             outR += y[i];
        }
        outL *= OUT_SCALE;
        outR *= OUT_SCALE;

        buffer.setSample(0, sampleIdx, outL);
        if (numChannels >= 2) buffer.setSample(1, sampleIdx, outR);
    }
}

void FDNReverb::reset() {
    for (size_t i = 0; i < static_cast<size_t>(N); ++i) {
        delayLines[i].clear();
        dampFilters[i].reset();
        modLFOs[i].reset();
    }
    diffusion.reset();
    freezeBlend.setCurrentAndTargetValue(frozen ? 1.0f : 0.0f);
    flatBlend.setCurrentAndTargetValue(flatEnabledFlag ? 1.0f : 0.0f);
}
