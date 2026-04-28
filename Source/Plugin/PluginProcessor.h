#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Input/GainModule.h"
#include "Input/Saturation.h"
#include "Input/TimbreFilter.h"
#include "Modulation/Vibrato.h"
#include "Input/InputFilter.h"
#include "Input/Predelay.h"
#include "EarlyReflections/EarlyReflections.h"
#include "Decay/FDNReverb.h"
#include "DiffusionNetwork/Chorus.h"
#include "Output/StereoWidener.h"
#include "Output/DryWetMixer.h"

class AudioPluginAudioProcessor final : public juce::AudioProcessor {
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    // DSP chain
    GainModule       gainModule;
    Saturation       saturationModule;
    TimbreFilter     timbreFilter;
    Vibrato          vibratoModule;
    InputFilter      inputFilter;
    Predelay         predelayModule;
    EarlyReflections earlyReflections;
    FDNReverb        fdnReverb;
    Chorus           chorusModule;
    StereoWidener    stereoWidener;
    DryWetMixer      dryWetMixer;

    // Separate buffer for ER-only output (B1 fix: keeps ER and FDN gains independent)
    juce::AudioBuffer<float> erOutputBuffer;

    // Smooth mode tracking (avoids resetting smoother every block)
    int lastSmoothMode = -1;

    // Smoothed parameter values to avoid clicks
    juce::SmoothedValue<float> smoothDecay, smoothDiffusion, smoothSize;
    juce::SmoothedValue<float> smoothDamping, smoothFeedback;
    juce::SmoothedValue<float> smoothPredelay, smoothStereo, smoothDryWet;
    juce::SmoothedValue<float> smoothChorusAmount, smoothChorusRate;
    juce::SmoothedValue<float> smoothCrossoverFreq;
    juce::SmoothedValue<float> smoothReflectGain, smoothDiffuseGain;
    juce::SmoothedValue<float> smoothErAmount, smoothErRate;
    juce::SmoothedValue<float> smoothLoCutFreq, smoothHiCutFreq;

    void updateDSPParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
