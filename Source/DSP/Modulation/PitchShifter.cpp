#include "PitchShifter.h"

// ---------------------------------------------------------------------------
// public
// ---------------------------------------------------------------------------

float PitchShifter::computeShift(float hz) const noexcept {
    float shift = hz - kCenterHz;  // linear Hz shift; 0 at 440 Hz
    if (octaveStep)
        shift = std::round(shift / kCenterHz) * kCenterHz;  // snap to 0, ±440, ±880 Hz
    return shift;
}

void PitchShifter::prepare(double sampleRate) {
    sr    = sampleRate;
    phase = 0.0f;
    smoothShift.reset(sampleRate, 0.05);
    smoothShift.setCurrentAndTargetValue(computeShift(targetHz));
    for (auto& h : hilbert) h.reset();
}

void PitchShifter::reset() {
    phase = 0.0f;
    smoothShift.setCurrentAndTargetValue(computeShift(targetHz));
    for (auto& h : hilbert) h.reset();
}

void PitchShifter::setFrequency(float hz) {
    targetHz = hz;
    const float s = computeShift(hz);
    if (octaveStep)
        smoothShift.setCurrentAndTargetValue(s);
    else
        smoothShift.setTargetValue(s);
}

void PitchShifter::setOctaveStep(bool on) {
    octaveStep = on;
    const float s = computeShift(targetHz);
    if (on)
        smoothShift.setCurrentAndTargetValue(s);
    else
        smoothShift.setTargetValue(s);
}

// ---------------------------------------------------------------------------
// process — single-sideband frequency shift
//
// For each sample:
//   I = input passed through 4-stage all-pass chain A
//   Q = input passed through 4-stage all-pass chain B  (≈90° phase lag vs I)
//   y = I·cos(θ) − Q·sin(θ)          (upper sideband: shifts all freqs up by shiftHz)
//   θ += 2π·shiftHz / sr             (negative shiftHz shifts down naturally)
//
// Both channels share the same phasor so the stereo image stays stable.
// ---------------------------------------------------------------------------

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int   numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int   numSamples  = buffer.getNumSamples();
    const float twoPi       = juce::MathConstants<float>::twoPi;
    const float srF         = static_cast<float>(sr);

    for (int s = 0; s < numSamples; ++s) {
        const float shiftHz = smoothShift.getNextValue();
        const float c       = std::cos(phase);
        const float sn      = std::sin(phase);

        for (int ch = 0; ch < numChannels; ++ch) {
            auto&  h    = hilbert[ch];
            float* data = buffer.getWritePointer(ch);
            const float x = data[s];

            float I = x;
            I = h.pathA[0].process(I, kCoeffA[0]);
            I = h.pathA[1].process(I, kCoeffA[1]);
            I = h.pathA[2].process(I, kCoeffA[2]);
            I = h.pathA[3].process(I, kCoeffA[3]);

            float Q = x;
            Q = h.pathB[0].process(Q, kCoeffB[0]);
            Q = h.pathB[1].process(Q, kCoeffB[1]);
            Q = h.pathB[2].process(Q, kCoeffB[2]);
            Q = h.pathB[3].process(Q, kCoeffB[3]);

            data[s] = I * c - Q * sn;
        }

        phase += twoPi * shiftHz / srF;
        if (phase >  twoPi) phase -= twoPi;
        if (phase < -twoPi) phase += twoPi;
    }
}
