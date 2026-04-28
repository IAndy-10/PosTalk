#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// Mixes a dry (input) buffer with a wet (processed) buffer.
// mix = 0.0 → all dry; mix = 1.0 → all wet.
// Per-sample interpolation smooths transitions when the knob changes quickly.
class DryWetMixer {
public:
    void setMix(float mixAmount) {
        previous = mix;
        mix = mixAmount;
    }

    // Call once during plugin prepare: pre-allocates the dry buffer.
    void prepare(int numChannels, int maxBlockSize) {
        dryBuffer.setSize(numChannels, maxBlockSize, false, true, false);
    }

    // Call before processing: saves the dry signal.
    void saveDry(const juce::AudioBuffer<float>& input) {
        int channels = input.getNumChannels();
        int samples  = input.getNumSamples();

        // Resize without reallocating if already large enough.
        dryBuffer.setSize(channels, samples, false, false, true);
        for (int ch = 0; ch < channels; ++ch)
            dryBuffer.copyFrom(ch, 0, input, ch, 0, samples);
    }

    // Call after processing: blends dry and wet into output with per-sample smoothing.
    void mixToOutput(juce::AudioBuffer<float>& wetOutput) {
        int channels = juce::jmin(wetOutput.getNumChannels(), dryBuffer.getNumChannels());
        int samples  = juce::jmin(wetOutput.getNumSamples(),  dryBuffer.getNumSamples());

        // Increment per sample so we ramp smoothly from previous mix to current mix.
        float increment = (samples > 0) ? (mix - previous) / static_cast<float>(samples) : 0.0f;

        for (int ch = 0; ch < channels; ++ch) {
            auto* wet = wetOutput.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < samples; ++i) {
                float w = previous + increment * static_cast<float>(i);
                wet[i] = (1.0f - w) * dry[i] + w * wet[i];
            }
        }

        // Advance previous so the next block starts from where we left off.
        previous = mix;
    }

private:
    float previous = 0.5f;
    float mix      = 0.5f;
    juce::AudioBuffer<float> dryBuffer;
};

