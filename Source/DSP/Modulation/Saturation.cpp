#include "Saturation.h"

void Saturation::prepare(double sampleRate) {
    sr = static_cast<float>(sampleRate);
    auto init = [&](juce::SmoothedValue<float>& sv, float v) {
        sv.reset(sampleRate, 0.01);
        sv.setCurrentAndTargetValue(v);
    };
    init(smoothDrive, 0.0f);
    init(smoothMix,   0.0f);
    init(smoothTone,  1.0f);
    aaState.fill(0.0f);
    toneState.fill(0.0f);
    prevSample.fill(0.0f);
}

void Saturation::reset() {
    aaState.fill(0.0f);
    toneState.fill(0.0f);
    prevSample.fill(0.0f);
}

void Saturation::setDrive(float v)  { smoothDrive.setTargetValue(v); }           // 0–1
void Saturation::setMix(float pct)  { smoothMix.setTargetValue(pct / 100.0f); }  // 0–100 → 0–1
void Saturation::setTone(float v)   { smoothTone.setTargetValue(v); }            // 0–1

void Saturation::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Tone coefficient: per block is fine (filter cutoff, not direct amplitude)
    const float tone  = smoothTone.skip(numSamples);
    const float fc    = 500.0f + 19500.0f * tone; // 500–20000 Hz
    const float kTone = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * fc / sr); // One-pole LPF coefficient for post-saturation tone control
    constexpr float kAA = 0.3f; // One-pole LPF coefficient for anti-aliasing the waveshaper output

    // Outer loop: samples — smoothDrive / smoothMix advance once per sample,
    // both channels share the same smoothed value (matches GainModule pattern).
    for (int s = 0; s < numSamples; ++s) {
        const float drv    = smoothDrive.getNextValue();
        const float mix    = smoothMix.getNextValue();
        // Equal-power wet/dry gains: constant perceived loudness across the blend range
        const float wetGain = std::sqrt(mix);
        const float dryGain = std::sqrt(1.0f - mix);

        // Waveshaper drive: 0 → identity (d≈0.001), 1 → heavy clip (d≈10)
        const float d     = 0.001f + drv * 9.999f;
        const float tanhD = std::tanh(d);

        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch) {
            const float dry = buffer.getWritePointer(ch)[s];

            // 2× virtual oversampling:
            //   upsample via linear interp → shape both sub-samples → average downsample
            const float up0 = 0.5f * (prevSample[ch] + dry);
            const float w0  = std::tanh(up0 * d) / tanhD;
            const float w1  = std::tanh(dry  * d) / tanhD;
            float       wet = 0.5f * (w0 + w1);

            // Anti-aliasing one-pole LP (k≈0.3)
            aaState[ch] += kAA * (wet - aaState[ch]);

            // Post-saturation tone LP
            toneState[ch] += kTone * (aaState[ch] - toneState[ch]);

            // Equal-power wet/dry mix
            buffer.getWritePointer(ch)[s] = dryGain * dry + wetGain * toneState[ch];
            prevSample[ch] = dry;
        }
    }
}
