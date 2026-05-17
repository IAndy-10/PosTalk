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
    if (octaveStep) {
        smoothShift.setCurrentAndTargetValue(s);
    } else if (s != smoothShift.getTargetValue()) {
        // Only restart the ramp when the target shift actually changes.
        // Calling setTargetValue with the same target every block resets the countdown
        // each time, converting the intended 50 ms linear ramp into an exponential
        // decay that can keep the effect audible for 300–400 ms.
        smoothShift.setTargetValue(s);
    }
}

void PitchShifter::setOctaveStep(bool on) {
    octaveStep = on;
    const float s = computeShift(targetHz);
    if (on) {
        smoothShift.setCurrentAndTargetValue(s);
    } else if (s != smoothShift.getTargetValue()) {
        smoothShift.setTargetValue(s);
    }
}

// ---------------------------------------------------------------------------
// process — single-sideband frequency shift with dry bypass
//
// For each sample:
//   I = input passed through 4-stage all-pass chain A
//   Q = input passed through 4-stage all-pass chain B  (≈90° phase lag vs I)
//   y = I·cos(θ) − Q·sin(θ)          (upper sideband: shifts all freqs up by shiftHz)
//   θ += 2π·shiftHz / sr             (negative shiftHz shifts down naturally)
//
// Both channels share the same phasor so the stereo image stays stable.
//
// Bypass crossfade:
//   When |shiftHz| < kBypassThreshold the output fades to the original dry
//   signal so that 440 Hz is truly transparent. The all-pass chains still run
//   at every sample so their IIR state stays warm — no transient when shift
//   re-engages. At shiftHz == 0 the phasor is also reset to 0 so the next
//   activation always starts from a clean angle.
// ---------------------------------------------------------------------------

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int   numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int   numSamples  = buffer.getNumSamples();
    const float twoPi       = juce::MathConstants<float>::twoPi;
    const float srF         = static_cast<float>(sr);

    // Crossfade region: full Hilbert above this, pure dry at 0 Hz.
    // At 44.1 kHz the 50 ms smooth ramp spends ~22 samples below 10 Hz —
    // inaudible on a diffuse reverb tail.
    static constexpr float kBypassThreshold = 10.0f;

    for (int s = 0; s < numSamples; ++s) {
        const float shiftHz = smoothShift.getNextValue();

        if (shiftHz == 0.0f)
            phase = 0.0f;

        const float c     = std::cos(phase);
        const float sn    = std::sin(phase);
        const float blend = std::min(std::abs(shiftHz) / kBypassThreshold, 1.0f);

        for (int ch = 0; ch < numChannels; ++ch) {
            auto&  h    = hilbert[ch];
            float* data = buffer.getWritePointer(ch);
            const float x = data[s];   // original sample — saved before all-pass

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

            // blend == 1: full shift effect; blend == 0: transparent dry pass-through
            data[s] = blend * (I * c - Q * sn) + (1.0f - blend) * x;
        }

        phase += twoPi * shiftHz / srF;
        if (phase >  twoPi) phase -= twoPi;
        if (phase < -twoPi) phase += twoPi;
    }
}
