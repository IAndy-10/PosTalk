#include "TimbreFilter.h"

void TimbreFilter::prepare(double sampleRate) {
    sr = sampleRate;
    auto init = [&](juce::SmoothedValue<float>& sv, float v) {
        sv.reset(sampleRate, 0.01);
        sv.setCurrentAndTargetValue(v);
    };
    init(smoothCutoff,    8000.0f);
    init(smoothResonance, 0.7071f);
    init(smoothDrivePre,  1.0f);   // linear gain — 0 dB default
    for (auto& ch : state) ch.fill(0.0);
}

void TimbreFilter::reset() {
    for (auto& ch : state) ch.fill(0.0);
}

void TimbreFilter::setCutoff(float hz)     { smoothCutoff.setTargetValue(hz); }
void TimbreFilter::setResonance(float q)   { smoothResonance.setTargetValue(q); }
void TimbreFilter::setDrivePreDb(float dB) { smoothDrivePre.setTargetValue(juce::Decibels::decibelsToGain(dB)); }

void TimbreFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Coefficients computed once per block — cutoff/resonance smooth across blocks via skip()
    const float cutoff = juce::jlimit(20.0f, (float)(sr * 0.49), smoothCutoff.skip(numSamples));
    const float q      = juce::jlimit(0.5f,  10.0f,              smoothResonance.skip(numSamples));

    // Biquad LP — Audio EQ Cookbook, Direct Form 2 Transposed, double-precision state
    const double omega = juce::MathConstants<double>::twoPi * cutoff / sr;
    const double sinW  = std::sin(omega);
    const double cosW  = std::cos(omega);
    const double alpha = sinW / (2.0 * q);
    const double a0    = 1.0 + alpha;
    const double nb0   = (1.0 - cosW) * 0.5 / a0;
    const double nb1   = (1.0 - cosW)       / a0;
    const double nb2   = (1.0 - cosW) * 0.5 / a0;
    const double na1   = (-2.0 * cosW)       / a0;
    const double na2   = (1.0 - alpha)        / a0;

    // Outer loop: samples — pre-drive advances per-sample; inner: channels
    for (int s = 0; s < numSamples; ++s) {
        const float drive = smoothDrivePre.getNextValue();

        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
            const double x  = buffer.getWritePointer(ch)[s] * drive;
            const double y  = nb0 * x + state[ch][0];
            state[ch][0]    = nb1 * x - na1 * y + state[ch][1];
            state[ch][1]    = nb2 * x - na2 * y;
            buffer.getWritePointer(ch)[s] = static_cast<float>(y);
        }
    }
}
