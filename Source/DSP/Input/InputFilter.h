#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// Stereo biquad input filter: high-pass (Lo Cut) and low-pass (Hi Cut).
// Both filters use Butterworth response (Q = 0.7071).
class InputFilter {
public:
    void prepare(double sampleRate);

    void setLoCutFreq(float hz);    // High-pass frequency
    void setHiCutFreq(float hz);    // Low-pass frequency
    void setHiCutQ(float q);        // Low-pass resonance Q (default 0.7071 = Butterworth)
    void setLoCutEnabled(bool on);
    void setHiCutEnabled(bool on);

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    void updateHP();
    void updateLP();

    void processChannel(float* data, int numSamples,
                        double b0, double b1, double b2,
                        double a1, double a2,
                        double& z1, double& z2);

    double sr = 44100.0;
    float  loCutHz  = 80.0f;
    float  hiCutHz  = 8000.0f;
    float  hiCutQVal = 0.7071f;    // default: Butterworth
    bool   loCutOn  = false;
    bool   hiCutOn  = false;

    // HP biquad coefficients (Lo Cut)
    double hp_b0 = 1, hp_b1 = 0, hp_b2 = 0, hp_a1 = 0, hp_a2 = 0;
    double hp_z1[2] = {}, hp_z2[2] = {};

    // LP biquad coefficients (Hi Cut)
    double lp_b0 = 1, lp_b1 = 0, lp_b2 = 0, lp_a1 = 0, lp_a2 = 0;
    double lp_z1[2] = {}, lp_z2[2] = {};
};

