#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>

// Granular pitch shifter with optional octave-snap mode.
//
// setFrequency(hz): maps the target pitch — 440 Hz = unity (no shift),
//   880 Hz = +1 octave, 220 Hz = -1 octave (range: 110–1760 Hz = ±2 octaves).
//
// setOctaveStep(on): when ON, the ratio is snapped to the nearest integer
//   power of 2 (i.e., whole-octave steps: 0.25×, 0.5×, 1×, 2×, 4×).
//   Transitions jump immediately rather than sweeping through intermediate ratios.
//   When OFF, the ratio changes smoothly (50 ms ramp).

class PitchShifter {
public:
    // Grain size and look-behind buffer size in milliseconds.
    // Larger grainMs → smoother pitch, more latency; smaller → more artifact.
    static constexpr float kGrainMs   = 80.0f;
    static constexpr float kMaxBufMs  = 300.0f;
    // Reference frequency that maps to unity ratio (no pitch shift)
    static constexpr float kCenterHz  = 440.0f;

    void prepare(double sampleRate);

    void setFrequency(float hz);   // 440 = unity, 880 = +1 oct, 220 = -1 oct
    void setOctaveStep(bool on);   // snap ratio to nearest power-of-2 octave

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    void  processChannel(float* data, int numSamples, const float* ratios, int ch);
    float computeRatio(float hz) const;

    double sr          = 44100.0;
    bool   octaveStep  = false;
    float  targetHz    = kCenterHz;

    // Two overlapping grain readers per channel (50% overlap, Hanning envelope).
    // Sum of two 50%-overlapping Hanning windows = 1.0 → no amplitude ripple.
    struct GrainState {
        std::vector<float> buf;
        int   writeIdx       = 0;
        float grainPhase[2]  = { 0.0f, 0.5f };  // normalized position in grain [0,1)
        float readPos[2]     = { 0.0f, 0.0f };  // absolute read position in buf[]
    };
    std::array<GrainState, 2> grains;  // index = channel (0=L, 1=R)

    int grainSize = 0;  // samples
    int bufSize   = 0;  // samples

    juce::SmoothedValue<float> smoothRatio { 1.0f };
};
