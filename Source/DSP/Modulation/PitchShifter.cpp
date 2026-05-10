#include "PitchShifter.h"

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static inline float wrapBuf(float pos, int size) {
    const float s = (float)size;
    while (pos >= s) pos -= s;
    while (pos <  0) pos += s;
    return pos;
}

// ---------------------------------------------------------------------------
// public
// ---------------------------------------------------------------------------

void PitchShifter::prepare(double sampleRate) {
    sr        = sampleRate;
    grainSize = (int)(kGrainMs  * sampleRate / 1000.0);
    bufSize   = (int)(kMaxBufMs * sampleRate / 1000.0) + grainSize + 4;

    for (int ch = 0; ch < 2; ++ch) {
        auto& g = grains[ch];
        g.buf.assign(bufSize, 0.0f);
        g.writeIdx      = 0;
        g.grainPhase[0] = 0.0f;
        g.grainPhase[1] = 0.5f;  // grain 1 starts half-grain ahead in phase
        // Place readers at the corresponding positions behind the write head
        g.readPos[0] = 0.0f;
        g.readPos[1] = wrapBuf(-(float)grainSize * 0.5f, bufSize);
    }

    smoothRatio.reset(sampleRate, 0.05);  // 50 ms ramp for continuous mode
    smoothRatio.setCurrentAndTargetValue(computeRatio(targetHz));
}

void PitchShifter::reset() {
    for (int ch = 0; ch < 2; ++ch) {
        auto& g = grains[ch];
        std::fill(g.buf.begin(), g.buf.end(), 0.0f);
        g.writeIdx      = 0;
        g.grainPhase[0] = 0.0f;
        g.grainPhase[1] = 0.5f;
        g.readPos[0]    = 0.0f;
        g.readPos[1]    = wrapBuf(-(float)grainSize * 0.5f, bufSize);
    }
    smoothRatio.setCurrentAndTargetValue(computeRatio(targetHz));
}

float PitchShifter::computeRatio(float hz) const {
    float ratio = hz / kCenterHz;
    ratio = juce::jlimit(0.25f, 4.0f, ratio);  // ±2 octaves hard limit

    if (octaveStep) {
        // Snap to nearest octave: 2^round(log2(ratio))
        // e.g. ratio 0.7 → round(log2(0.7)=−0.51) → −1 → 0.5×
        //      ratio 1.3 → round(log2(1.3)=+0.38) →  0 → 1.0×
        const float octaves = std::round(std::log2(ratio));
        ratio = std::pow(2.0f, octaves);
    }

    return ratio;
}

void PitchShifter::setFrequency(float hz) {
    targetHz = hz;
    const float r = computeRatio(hz);
    if (octaveStep)
        smoothRatio.setCurrentAndTargetValue(r);  // instant snap, no sweep
    else
        smoothRatio.setTargetValue(r);            // 50 ms smooth glide
}

void PitchShifter::setOctaveStep(bool on) {
    octaveStep = on;
    const float r = computeRatio(targetHz);
    if (on)
        smoothRatio.setCurrentAndTargetValue(r);  // snap immediately on mode switch
    else
        smoothRatio.setTargetValue(r);
}

// ---------------------------------------------------------------------------
// process
// ---------------------------------------------------------------------------

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Advance the smoother once per sample so both channels share the same
    // ratio trajectory (avoid double-stepping the smoother in stereo).
    jassert(numSamples <= 4096);
    float ratios[4096];
    for (int s = 0; s < numSamples; ++s)
        ratios[s] = smoothRatio.getNextValue();

    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        processChannel(buffer.getWritePointer(ch), numSamples, ratios, ch);
}

// ---------------------------------------------------------------------------
// processChannel — granular pitch shift
//
// Algorithm: two-grain overlap-add.
//   • Each grain reader has a normalised phase φ ∈ [0,1).
//   • Grain 0 starts at φ=0, grain 1 at φ=0.5 (half-grain offset).
//   • Envelope: Hanning  w(φ) = 0.5·(1 − cos(2π·φ)).
//   • Two 50%-overlapping Hannings sum to exactly 1 → no amplitude ripple.
//   • Read pointer advances at `ratio` samples per sample; write at 1/sample.
//   • When a grain completes one full cycle (φ wraps), the read pointer
//     resyncs to half-grain behind the current write head, keeping latency
//     bounded and preventing drift regardless of ratio.
// ---------------------------------------------------------------------------

void PitchShifter::processChannel(float* data, int numSamples,
                                   const float* ratios, int ch) {
    auto& g           = grains[ch];
    const int   bSz   = bufSize;
    const float bSzF  = (float)bSz;
    const float gsF   = (float)grainSize;
    const float halfGs = gsF * 0.5f;
    const float phaseInc = 1.0f / gsF;  // grain phase advance per sample

    for (int s = 0; s < numSamples; ++s) {
        const float r = ratios[s];

        // Write input into circular buffer
        g.buf[g.writeIdx] = data[s];

        float out      = 0.0f;
        float totalEnv = 0.0f;

        for (int gi = 0; gi < 2; ++gi) {
            // Advance grain phase (normalised 0→1)
            g.grainPhase[gi] += phaseInc;

            if (g.grainPhase[gi] >= 1.0f) {
                // Grain cycle complete — wrap phase and resync read pointer.
                // Resyncing to half-grain behind write keeps the look-behind
                // window centred and prevents unbounded drift.
                g.grainPhase[gi] -= 1.0f;
                g.readPos[gi] = wrapBuf((float)g.writeIdx - halfGs, bSz);
            }

            // Hanning envelope — peaks at φ=0.5, zero at φ=0 and φ=1.
            // Property: H(φ) + H(φ+0.5) = 1  for all φ  (50% OLA identity).
            const float phi = g.grainPhase[gi];
            const float env = 0.5f * (1.0f - std::cos(phi * juce::MathConstants<float>::twoPi));

            // Linear interpolation read from circular buffer
            const float rp   = g.readPos[gi];
            const int   i0   = (int)rp % bSz;
            const float frac = rp - std::floor(rp);
            const int   i1   = (i0 + 1) % bSz;
            const float smp  = g.buf[i0] * (1.0f - frac) + g.buf[i1] * frac;

            out      += env * smp;
            totalEnv += env;

            // Advance read pointer at pitch ratio speed
            g.readPos[gi] = wrapBuf(g.readPos[gi] + r, bSz);
        }

        // Normalise: guards against the brief moment after init when both
        // envelopes are near zero before the first grain cycle completes.
        data[s] = (totalEnv > 1e-4f) ? out / totalEnv : 0.0f;

        g.writeIdx = (g.writeIdx + 1) % bSz;
    }
}
