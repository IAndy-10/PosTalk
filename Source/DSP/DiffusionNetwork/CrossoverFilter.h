#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// First-order one-pole lowpass (or high shelf) used per-delay-line in the FDN
// to achieve frequency-dependent decay. Operates on a single channel (mono).
//
// Filter modes (controlled by setFilterType):
//   LP mode  (default, useShelving=false):
//     y[n] = coeff * x[n] + (1 - coeff) * y[n-1]
//     Heavy damping → hard low-pass cut of high frequencies.
//
//   Shelf mode (useShelving=true):
//     y[n] = SHELF_GAIN * x[n] + (1 - SHELF_GAIN) * z1_lp
//     Reduces (but does not fully cut) high frequencies above the corner.
//     The damping parameter has NO effect in this mode (fixed shelf depth).
//     Corner frequency is still controlled by setCutoffFreq.
class CrossoverFilter {
public:
    void prepare(double sampleRate);
    void setCutoffFreq(float hz);
    void setDamping(float d);       // 0 = no damping, 1 = heavy damping (LP mode only)
    void setFilterType(bool shelving); // false = LP (default), true = high shelf

    // Process a single sample; returns filtered value
    float processSample(float in);

    void reset();

private:
    void updateCoeff();

    // Fixed shelf gain: HF is attenuated to this fraction above the corner frequency.
    // Approx -6 dB. Damping parameter is ignored in shelf mode.
    static constexpr float SHELF_GAIN = 0.5f;

    double sr = 44100.0;
    float  cutoff     = 3000.0f;
    float  damping    = 0.5f;
    bool   useShelving = false;

    double coeff = 0.5; // one-pole LP coefficient (shared by both modes)
    double z1    = 0.0;
};

