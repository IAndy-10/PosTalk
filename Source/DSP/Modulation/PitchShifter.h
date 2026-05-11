#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

// Frequency shifter via IIR Hilbert transform + single-sideband modulation.
//
// Replaces the previous granular pitch shifter. No grain boundaries, no
// periodic resync events — produces a clean, artifact-free frequency shift.
//
// Trade-off vs. pitch shifting: the shift is linear in Hz, not a ratio.
// Every frequency bin moves by the same Hz amount, so harmonic relationships
// change. On a diffuse reverb tail this blends naturally as shimmer texture.
//
// setFrequency(hz): 440 Hz = no shift.  >440 Hz shifts up, <440 Hz shifts down.
//   Shift amount = hz − 440.  e.g. 880 Hz → +440 Hz shift, 220 Hz → −220 Hz.
//   Parameter range (110–1760 Hz) maps to −330 … +1320 Hz shift.
//
// setOctaveStep(on): snaps shift to nearest multiple of 440 Hz
//   (0, ±440, ±880 Hz) for musically-spaced jumps.
//
// Algorithm:
//   1. Split input into I (in-phase) and Q (quadrature, ≈90° shifted) using
//      two complementary 4-stage IIR all-pass chains (wideband Hilbert approx,
//      <1° error from 20 Hz to 20 kHz at 44.1 kHz).
//   2. y = I·cos(θ) − Q·sin(θ), where θ advances by 2π·shiftHz/sr per sample.
//      Positive shiftHz → upper sideband (shift up); negative → shift down.
//   Both channels share the same phasor for a stable stereo image.

class PitchShifter {
public:
    static constexpr float kCenterHz = 440.0f;

    void prepare(double sampleRate);
    void setFrequency(float hz);
    void setOctaveStep(bool on);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    // First-order IIR all-pass: y[n] = −a·x[n] + x[n−1] + a·y[n−1]
    struct APF1 {
        float xp = 0.0f;
        float yp = 0.0f;
        float process(float x, float a) noexcept {
            const float y = -a * x + xp + a * yp;
            xp = x;  yp = y;
            return y;
        }
        void reset() noexcept { xp = yp = 0.0f; }
    };

    struct HilbertState {
        APF1 pathA[4];
        APF1 pathB[4];
        void reset() {
            for (auto& s : pathA) s.reset();
            for (auto& s : pathB) s.reset();
        }
    };

    // All-pass poles for wideband 90° phase splitter — tuned for 44.1 kHz.
    // pathA and pathB interleaved to cover 20 Hz – 20 kHz with <1° phase error.
    static constexpr float kCoeffA[4] = {
        0.4021921162f, 0.8561710323f, 0.9722909193f, 0.9952884791f
    };
    static constexpr float kCoeffB[4] = {
        0.6923878f, 0.9360654323f, 0.9882295227f, 0.9987488452f
    };

    std::array<HilbertState, 2> hilbert;  // [0]=L, [1]=R

    double sr         = 44100.0;
    float  phase      = 0.0f;     // shared phasor [0, 2π)
    bool   octaveStep = false;
    float  targetHz   = kCenterHz;

    juce::SmoothedValue<float> smoothShift { 0.0f };  // shift in Hz, smoothed 50 ms

    float computeShift(float hz) const noexcept;
};
