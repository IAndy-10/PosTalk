#include "InputFilter.h"
#include <cmath>

static constexpr double kPi = 3.14159265358979323846;
static constexpr double kSqrt2Inv = 0.70710678118; // 1/sqrt(2)

void InputFilter::prepare(double sampleRate) {
    sr = sampleRate;
    updateHP();
    updateLP();
    reset();
}

void InputFilter::setLoCutFreq(float hz) {
    loCutHz = hz;
    updateHP();
}

void InputFilter::setHiCutFreq(float hz) {
    hiCutHz = hz;
    updateLP();
}

void InputFilter::setHiCutQ(float q) {
    hiCutQVal = q;
    updateLP();
}

void InputFilter::setLoCutEnabled(bool on) { loCutOn = on; }
void InputFilter::setHiCutEnabled(bool on) { hiCutOn = on; }

void InputFilter::reset() {
    for (int ch = 0; ch < 2; ++ch) {
        hp_z1[ch] = hp_z2[ch] = 0.0;
        lp_z1[ch] = lp_z2[ch] = 0.0;
    }
}

// Butterworth 2nd-order high-pass biquad
void InputFilter::updateHP() {
    double omega = 2.0 * kPi * static_cast<double>(loCutHz) / sr;
    double cos_w = std::cos(omega);
    double sin_w = std::sin(omega);
    double alpha = sin_w / (2.0 * kSqrt2Inv);

    double a0 =  1.0 + alpha;
    hp_b0 =  (1.0 + cos_w) / (2.0 * a0);   // HPF: numerator uses (1 + cos_w), not (1 - cos_w)
    hp_b1 = -(1.0 + cos_w) / a0;
    hp_b2 =  (1.0 + cos_w) / (2.0 * a0);
    hp_a1 = -2.0 * cos_w / a0;
    hp_a2 =  (1.0 - alpha) / a0;
}

// 2nd-order low-pass biquad with variable Q
void InputFilter::updateLP() {
    double omega = 2.0 * kPi * static_cast<double>(hiCutHz) / sr;
    double cos_w = std::cos(omega);
    double sin_w = std::sin(omega);
    double alpha = sin_w / (2.0 * static_cast<double>(hiCutQVal));

    double a0 = 1.0 + alpha;
    lp_b0 = (1.0 - cos_w) / (2.0 * a0);
    lp_b1 = (1.0 - cos_w) / a0;
    lp_b2 = (1.0 - cos_w) / (2.0 * a0);
    lp_a1 = -2.0 * cos_w / a0;
    lp_a2 = (1.0 - alpha) / a0;
}

void InputFilter::processChannel(float* data, int numSamples,
                                  double b0, double b1, double b2,
                                  double a1, double a2,
                                  double& z1, double& z2) {
    // ScopedNoDenormals (set in processBlock) handles SSE FTZ for floats.
    // For double-precision state vars we add an explicit sub-denormal guard:
    // values below ~1e-15 are vanishingly small and safe to zero.
    static constexpr double kDenormalFloor = 1.0e-15;

    for (int i = 0; i < numSamples; ++i) {
        double in  = static_cast<double>(data[i]);
        double out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;

        // Flush near-zero state to prevent denormal CPU stalls on quiet tails.
        if (z1 < kDenormalFloor && z1 > -kDenormalFloor) z1 = 0.0;
        if (z2 < kDenormalFloor && z2 > -kDenormalFloor) z2 = 0.0;

        data[i] = static_cast<float>(out);
    }
}

void InputFilter::process(juce::AudioBuffer<float>& buffer) {
    int channels   = juce::jmin(buffer.getNumChannels(), 2);
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < channels; ++ch) {
        float* data = buffer.getWritePointer(ch);

        if (loCutOn)
            processChannel(data, numSamples, hp_b0, hp_b1, hp_b2, hp_a1, hp_a2, hp_z1[ch], hp_z2[ch]);

        if (hiCutOn)
            processChannel(data, numSamples, lp_b0, lp_b1, lp_b2, lp_a1, lp_a2, lp_z1[ch], lp_z2[ch]);
    }
}

