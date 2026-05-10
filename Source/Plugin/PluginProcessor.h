#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Input/GainModule.h"
#include "Modulation/Saturation.h"
#include "Modulation/TimbreFilter.h"
#include "Modulation/Vibrato.h"
#include "Modulation/PitchShifter.h"
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
    InputFilter      inputFilter;
    Predelay         predelayModule;
    EarlyReflections earlyReflections;
    FDNReverb        fdnReverb;
    // Tail modulation (shimmer section — post-FDN)
    Vibrato          vibratoModule;
    PitchShifter     pitchShifter;
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
    juce::SmoothedValue<float> smoothFdnInputGain;  // ramps gain when frozen state changes
    juce::SmoothedValue<float> smoothCrossoverFreq;
    juce::SmoothedValue<float> smoothReflectGain, smoothDiffuseGain;
    juce::SmoothedValue<float> smoothErAmount, smoothErRate;
    juce::SmoothedValue<float> smoothLoCutFreq, smoothHiCutFreq;

    // ── Cached APVTS raw-parameter pointers (resolved once in prepareToPlay) ──
    // Avoids per-block hash-map lookups in getRawParameterValue().
    std::atomic<float>* pSmooth          = nullptr;
    std::atomic<float>* pGainDb          = nullptr;
    std::atomic<float>* pOutputTrimDb    = nullptr;
    std::atomic<float>* pSatDrive        = nullptr;
    std::atomic<float>* pSatMix          = nullptr;
    std::atomic<float>* pSatTone         = nullptr;
    std::atomic<float>* pTimbreCutoff    = nullptr;
    std::atomic<float>* pTimbreResonance = nullptr;
    std::atomic<float>* pTimbreDrivePre  = nullptr;
    std::atomic<float>* pLoCutEnabled    = nullptr;
    std::atomic<float>* pHiCutEnabled    = nullptr;
    std::atomic<float>* pLoCutFreq       = nullptr;
    std::atomic<float>* pHiCutFreq       = nullptr;
    std::atomic<float>* pHiCutQ          = nullptr;
    std::atomic<float>* pPredelay        = nullptr;
    std::atomic<float>* pErEnabled       = nullptr;
    std::atomic<float>* pErAmount        = nullptr;
    std::atomic<float>* pErRate          = nullptr;
    std::atomic<float>* pErShape         = nullptr;
    std::atomic<float>* pFreeze          = nullptr;
    std::atomic<float>* pDecay           = nullptr;
    std::atomic<float>* pDiffusion       = nullptr;
    std::atomic<float>* pSize            = nullptr;
    std::atomic<float>* pDamping         = nullptr;
    std::atomic<float>* pFeedback        = nullptr;
    std::atomic<float>* pCrossoverFreq   = nullptr;
    std::atomic<float>* pReverbMode      = nullptr;
    std::atomic<float>* pHighFilterType  = nullptr;
    std::atomic<float>* pScale           = nullptr;
    std::atomic<float>* pFlatEnabled     = nullptr;
    std::atomic<float>* pCutEnabled      = nullptr;
    std::atomic<float>* pDensity         = nullptr;
    std::atomic<float>* pReflectGain     = nullptr;
    std::atomic<float>* pDiffuseGain     = nullptr;
    std::atomic<float>* pChorusEnabled   = nullptr;
    std::atomic<float>* pChorusAmount    = nullptr;
    std::atomic<float>* pChorusRate      = nullptr;
    std::atomic<float>* pVibratoRate     = nullptr;
    std::atomic<float>* pVibratoDepth    = nullptr;
    std::atomic<float>* pVibratoFadeIn   = nullptr;
    std::atomic<float>* pPitchFrequency  = nullptr;
    std::atomic<float>* pPitchOctaveStep = nullptr;
    std::atomic<float>* pStereo          = nullptr;
    std::atomic<float>* pDryWet          = nullptr;
    std::atomic<float>* pSustainEnabled  = nullptr;
    std::atomic<float>* pCutNow          = nullptr;

    // ── Performance control state ──
    bool sustainFromMidi   = false;  // true while MIDI CC64 is held (value >= 64)
    bool lastSustainOutput = false;  // previous UI sustain state — edge-detect for CC64 emission

    enum class CutNowState { Idle, FadingOut, FadingIn };
    CutNowState cutNowState      = CutNowState::Idle;
    float       cutNowGain       = 1.0f;
    float       cutNowFadeOutRate = 0.0f;  // gain-units/sample (computed in prepareToPlay)
    float       cutNowFadeInRate  = 0.0f;
    float       lastCutNowValue  = 0.0f;   // previous value — rising edge detection

    // ── FDN dirty-flag cache (slow/stable params — only push to DSP on change) ──
    // Sentinel value -1 ensures all params are pushed on the very first block.
    float lastDiffusion      = -1.0f;
    float lastSize           = -1.0f;
    float lastDamping        = -1.0f;
    float lastFeedback       = -1.0f;
    float lastCrossoverFreq  = -1.0f;
    float lastReverbMode     = -1.0f;
    float lastHighFilterType = -1.0f;
    float lastScale          = -1.0f;
    float lastFlatEnabled    = -1.0f;
    float lastCutEnabled     = -1.0f;
    float lastDensity        = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
