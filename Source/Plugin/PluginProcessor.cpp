#include "PluginProcessor.h"
#include "../Parameters/ParameterLayout.h"
#include "../Parameters/ParameterIDs.h"
#include "PluginEditor.h"

using namespace PluginParamIDs;

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input",  juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

const juce::String AudioPluginAudioProcessor::getName() const { return JucePlugin_Name; }
bool AudioPluginAudioProcessor::acceptsMidi()  const { return true;  }
bool AudioPluginAudioProcessor::producesMidi() const { return false; }
bool AudioPluginAudioProcessor::isMidiEffect() const { return false; }
double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 60.0; }
int AudioPluginAudioProcessor::getNumPrograms() { return 1; }
int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }
void AudioPluginAudioProcessor::setCurrentProgram(int) {}
const juce::String AudioPluginAudioProcessor::getProgramName(int) { return {}; }
void AudioPluginAudioProcessor::changeProgramName(int, const juce::String&) {}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // ── Resolve all APVTS parameter pointers once ──
    // Subsequent processBlock calls use p***->load() with no hash-map overhead.
    auto resolve = [&](const juce::ParameterID& pid) {
        return apvts.getRawParameterValue(pid.getParamID());
    };
    pSmooth          = resolve(smooth);
    pGainDb          = resolve(gainDb);
    pOutputTrimDb    = resolve(outputTrimDb);
    pSatDrive        = resolve(satDrive);
    pSatMix          = resolve(satMix);
    pSatTone         = resolve(satTone);
    pTimbreCutoff    = resolve(timbreCutoff);
    pTimbreResonance = resolve(timbreResonance);
    pTimbreDrivePre  = resolve(timbreDrivePre);
    pLoCutEnabled    = resolve(loCutEnabled);
    pHiCutEnabled    = resolve(hiCutEnabled);
    pLoCutFreq       = resolve(loCutFreq);
    pHiCutFreq       = resolve(hiCutFreq);
    pHiCutQ          = resolve(hiCutQ);
    pPredelay        = resolve(predelay);
    pErEnabled       = resolve(erEnabled);
    pErAmount        = resolve(erAmount);
    pErRate          = resolve(erRate);
    pErShape         = resolve(erShape);
    pFreeze          = resolve(freeze);
    pDecay           = resolve(decay);
    pDiffusion       = resolve(diffusion);
    pSize            = resolve(size);
    pDamping         = resolve(damping);
    pFeedback        = resolve(feedback);
    pCrossoverFreq   = resolve(crossoverFreq);
    pReverbMode      = resolve(reverbMode);
    pHighFilterType  = resolve(highFilterType);
    pScale           = resolve(scale);
    pFlatEnabled     = resolve(flatEnabled);
    pCutEnabled      = resolve(cutEnabled);
    pDensity         = resolve(density);
    pReflectGain     = resolve(reflectGain);
    pDiffuseGain     = resolve(diffuseGain);
    pChorusEnabled   = resolve(chorusEnabled);
    pChorusAmount    = resolve(chorusAmount);
    pChorusRate      = resolve(chorusRate);
    pVibratoRate     = resolve(vibratoRate);
    pVibratoDepth    = resolve(vibratoDepth);
    pVibratoFadeIn   = resolve(vibratoFadeIn);
    pPitchFrequency  = resolve(pitchFrequency);
    pPitchOctaveStep = resolve(pitchOctaveStep);
    pStereo          = resolve(stereo);
    pDryWet          = resolve(dryWet);
    pSustainEnabled  = resolve(sustainEnabled);
    pCutNow          = resolve(cutNow);

    // Cut Now ramp rates: 50 ms fade-out, 20 ms fade-in
    const float sr = static_cast<float>(sampleRate);
    cutNowFadeOutRate = 1.0f / (0.050f * sr);
    cutNowFadeInRate  = 1.0f / (0.020f * sr);
    cutNowState      = CutNowState::Idle;
    cutNowGain       = 1.0f;
    lastCutNowValue  = 0.0f;
    sustainFromMidi  = false;
    lastSustainOutput = false;

    // Reset FDN dirty-flag cache — sentinel -1 forces all params to be pushed on the first block
    lastDiffusion = lastSize = lastDamping = lastCrossoverFreq
        = lastReverbMode = lastHighFilterType = lastScale
        = lastFlatEnabled = lastCutEnabled = lastDensity = -1.0f;

    // Prepare DSP modules
    gainModule.prepare(sampleRate);
    saturationModule.prepare(sampleRate);
    timbreFilter.prepare(sampleRate);
    inputFilter.prepare(sampleRate);
    vibratoModule.prepare(sampleRate);
    pitchShifter.prepare(sampleRate);
    predelayModule.prepare(sampleRate);
    earlyReflections.prepare(sampleRate);
    fdnReverb.prepare(sampleRate);
    chorusModule.prepare(sampleRate);
    stereoWidener.prepare(sampleRate);
    dryWetMixer.prepare(getTotalNumOutputChannels(), samplesPerBlock);
    erOutputBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    lastSmoothMode = -1;

    // Initialise smoothed values using cached pointers (10 ms ramp)
    auto initSmooth = [&](juce::SmoothedValue<float>& sv, float initial) {
        sv.reset(sampleRate, 0.01);
        sv.setCurrentAndTargetValue(initial);
    };
    initSmooth(smoothLoCutFreq,    pLoCutFreq->load());
    initSmooth(smoothHiCutFreq,    pHiCutFreq->load());
    initSmooth(smoothPredelay,     pPredelay->load());
    initSmooth(smoothErAmount,     pErAmount->load());
    initSmooth(smoothErRate,       pErRate->load());
    initSmooth(smoothReflectGain,  pReflectGain->load());
    initSmooth(smoothCrossoverFreq,pCrossoverFreq->load());
    initSmooth(smoothDiffusion,    pDiffusion->load());
    initSmooth(smoothDamping,      pDamping->load());
    initSmooth(smoothFeedback,     pFeedback->load());
    initSmooth(smoothDiffuseGain,  pDiffuseGain->load());
    initSmooth(smoothChorusAmount, pChorusAmount->load());
    initSmooth(smoothChorusRate,   pChorusRate->load());
    initSmooth(smoothSize,         pSize->load());
    initSmooth(smoothDecay,        pDecay->load());
    initSmooth(smoothStereo,       pStereo->load());
    initSmooth(smoothDryWet,       pDryWet->load());

    // FDN input gain smoother: 100 ms ramp so frozen↔unfrozen transitions are click-free
    smoothFdnInputGain.reset(sampleRate, 0.10);
    smoothFdnInputGain.setCurrentAndTargetValue(1.0f);

    // Feedback smoother: 50 ms ramp for click-free sustain engage/release.
    // Overrides the 10 ms default set by initSmooth above.
    smoothFeedback.reset(sampleRate, 0.05);
    smoothFeedback.setCurrentAndTargetValue(pFeedback->load());
}

void AudioPluginAudioProcessor::releaseResources() {
    saturationModule.reset();
    timbreFilter.reset();
    inputFilter.reset();
    predelayModule.reset();
    earlyReflections.reset();
    fdnReverb.reset();
    vibratoModule.reset();
    pitchShifter.reset();
    chorusModule.reset();
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    // Parse incoming MIDI CC64 (physical sustain pedal — already routed to instrument by DAW)
    for (const auto meta : midiMessages) {
        const auto msg = meta.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 64)
            sustainFromMidi = (msg.getControllerValue() >= 64);
    }

    // Emit CC64 for UI/gesture sustain on rising/falling edge only.
    // The physical pedal is handled by the DAW; this covers the gesture layer.
    // Downstream DAW routing: PosTalk MIDI output → instrument track → instrument holds note.
    const bool uiSustain = pSustainEnabled->load() > 0.5f;
    if (uiSustain != lastSustainOutput) {
        midiMessages.addEvent(
            juce::MidiMessage::controllerEvent(1, 64, uiSustain ? 127 : 0), 0);
        lastSustainOutput = uiSustain;
    }

    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = getTotalNumInputChannels(); i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // ── Smooth mode: update ramp times when selector changes ──
    const int smoothMode = static_cast<int>(pSmooth->load());
    if (smoothMode != lastSmoothMode) {
        static constexpr float kSmoothTimes[] = { 0.001f, 0.02f, 0.08f, 0.20f };
        const float t = kSmoothTimes[juce::jlimit(0, 3, smoothMode)];
        smoothDecay.reset(getSampleRate(), t);
        smoothReflectGain.reset(getSampleRate(), t);
        smoothDiffuseGain.reset(getSampleRate(), t);
        lastSmoothMode = smoothMode;
    }
    // smoothDecay target is set in the FDN block below (sustain may override it).
    smoothReflectGain.setTargetValue(pReflectGain->load());
    smoothDiffuseGain.setTargetValue(pDiffuseGain->load());

    // ─────────────────────────────────────────────────────────────────────────
    // INPUT CONDITIONING: Gain → Saturation → TimbreFilter
    // Shapes what goes into the reverb. saveDry captured here so the dry
    // signal heard in the mix includes this input processing.
    // ─────────────────────────────────────────────────────────────────────────

    gainModule.setGainDb(pGainDb->load());
    gainModule.setOutputTrimDb(pOutputTrimDb->load());
    gainModule.process(buffer);

    saturationModule.setDrive(pSatDrive->load());
    saturationModule.setMix(pSatMix->load());
    saturationModule.setTone(pSatTone->load());
    saturationModule.process(buffer);

    timbreFilter.setCutoff(pTimbreCutoff->load());
    timbreFilter.setResonance(pTimbreResonance->load());
    timbreFilter.setDrivePreDb(pTimbreDrivePre->load());
    timbreFilter.process(buffer);

    // saveDry here: dry = input-conditioned, not reverb or tail modulation
    dryWetMixer.saveDry(buffer);

    // ===== PANEL 1: INPUT =====
    inputFilter.setLoCutEnabled(pLoCutEnabled->load() > 0.5f);
    inputFilter.setHiCutEnabled(pHiCutEnabled->load() > 0.5f);
    smoothLoCutFreq.setTargetValue(pLoCutFreq->load());
    smoothHiCutFreq.setTargetValue(pHiCutFreq->load());
    inputFilter.setLoCutFreq(smoothLoCutFreq.skip(numSamples));
    inputFilter.setHiCutFreq(smoothHiCutFreq.skip(numSamples));
    inputFilter.setHiCutQ(pHiCutQ->load());
    inputFilter.process(buffer);

    predelayModule.setDelayMs(pPredelay->load());
    predelayModule.process(buffer);

    // ===== PANEL 2: EARLY REFLECTIONS (parallel path — B1 fix) =====
    earlyReflections.setEnabled(pErEnabled->load() > 0.5f);
    earlyReflections.setAmount((pErAmount->load() - 2.0f) / 53.0f);
    earlyReflections.setRate(pErRate->load());
    earlyReflections.setShape(pErShape->load());
    earlyReflections.processOut(buffer, erOutputBuffer);

    // ===== PANEL 3: DIFFUSION NETWORK + DECAY =====
    const bool sustainActive = sustainFromMidi || (pSustainEnabled->load() > 0.5f);
    fdnReverb.setFrozen(pFreeze->load() > 0.5f);
    // Sustain: push decay to maximum so the tail holds without freezing.
    // The FDN keeps running normally (filters, modulation, input all active)
    // — sounds like a continuously held note, not a static frozen pad.
    smoothDecay.setTargetValue(sustainActive ? 60000.0f : pDecay->load());
    fdnReverb.setDecayMs(smoothDecay.skip(numSamples));

    // Stable FDN parameters — only push to DSP when value actually changes.
    // Avoids redundant coefficient recalculation every block.
    if (auto v = pDiffusion->load();      v != lastDiffusion)     { fdnReverb.setDiffusion(v);                    lastDiffusion = v; }
    if (auto v = pSize->load();           v != lastSize)           { fdnReverb.setSize(v);                         lastSize = v; }
    if (auto v = pDamping->load();        v != lastDamping)        { fdnReverb.setDamping(v);                      lastDamping = v; }
    // Sustain also overrides feedback to 1.0 so normalDecayGain = g×1.0 ≈ 0.998.
    // Without this, normalDecayGain = g×feedback (e.g. 0.998×0.75 = 0.748) and the
    // tail decays in ~0.4 s regardless of the 60 s decay target.
    // smoothFeedback ramps over 50 ms so engage/release does not produce a gain click.
    smoothFeedback.setTargetValue(sustainActive ? 1.0f : pFeedback->load());
    fdnReverb.setFeedback(smoothFeedback.skip(numSamples));
    if (auto v = pCrossoverFreq->load();  v != lastCrossoverFreq)  { fdnReverb.setCrossoverFreq(v);                lastCrossoverFreq = v; }
    if (auto v = pReverbMode->load();     v != lastReverbMode)     { fdnReverb.setReverbMode(static_cast<int>(v)); lastReverbMode = v; }
    if (auto v = pHighFilterType->load(); v != lastHighFilterType) { fdnReverb.setHighFilterType(v > 0.5f);        lastHighFilterType = v; }
    if (auto v = pScale->load();          v != lastScale)          { fdnReverb.setInputScale(v);                   lastScale = v; }
    if (auto v = pFlatEnabled->load();    v != lastFlatEnabled)    { fdnReverb.setFlatEnabled(v > 0.5f);           lastFlatEnabled = v; }
    if (auto v = pCutEnabled->load();     v != lastCutEnabled)     { fdnReverb.setCutEnabled(v > 0.5f);            lastCutEnabled = v; }
    if (auto v = pDensity->load();        v != lastDensity)        { fdnReverb.setDensity(static_cast<int>(v));    lastDensity = v; }

    // ── Decay-proportional input scaling ──
    // Prevents FDN tank saturation when many notes overlap at long decay settings.
    // Uses the *effective* decay (60 s when sustain is active, knob value otherwise)
    // so the formula accounts for the actual per-line feedback gain in use.
    // Freeze forces -26 dB regardless; sustain lets the formula run naturally.
    // The 100 ms smoothed ramp ensures gain transitions are inaudible.
    {
        float targetInputGain;
        if (pFreeze->load() > 0.5f) {
            targetInputGain = 0.05f;  // -26 dB: tank is static, only let a trickle in
        } else {
            const float L_avg_sec    = 0.01772f
                                     * (static_cast<float>(getSampleRate()) / 44100.0f)
                                     * (0.5f + pSize->load());
            // Use effective decay and feedback — both may be overridden by sustain.
            const float effectDecayS = sustainActive ? 60.0f
                                                     : juce::jmax(0.2f, pDecay->load() / 1000.0f);
            const float fb           = sustainActive ? 1.0f : pFeedback->load();
            const float gNow         = std::pow(10.0f, -3.0f * L_avg_sec / effectDecayS) * fb;
            const float gRef         = std::pow(10.0f, -3.0f * L_avg_sec / 1.5f)         * fb;
            targetInputGain = juce::jmin(1.0f,
                                  (1.0f - juce::jmin(gNow, 0.9998f)) /
                                  juce::jmax(1.0f - juce::jmin(gRef, 0.9998f), 0.0001f));
        }
        smoothFdnInputGain.setTargetValue(targetInputGain);
        buffer.applyGain(smoothFdnInputGain.skip(numSamples));
    }

    fdnReverb.process(buffer);

    // Apply diffuseGain to FDN tail; add reflectGain-scaled ER separately (B1 fix)
    const float dGain = juce::Decibels::decibelsToGain(smoothDiffuseGain.skip(numSamples));
    const float rGain = juce::Decibels::decibelsToGain(smoothReflectGain.skip(numSamples));
    buffer.applyGain(dGain);
    for (int ch = 0; ch < numChannels; ++ch)
        buffer.addFrom(ch, 0, erOutputBuffer, ch, 0, numSamples, rGain);

    // ─────────────────────────────────────────────────────────────────────────
    // TAIL MODULATION (shimmer section): Vibrato → PitchShifter → Chorus
    // ─────────────────────────────────────────────────────────────────────────

    vibratoModule.setRate(pVibratoRate->load());
    vibratoModule.setDepth(pVibratoDepth->load());
    vibratoModule.setFadeInMs(pVibratoFadeIn->load());
    vibratoModule.process(buffer);

    pitchShifter.setFrequency(pPitchFrequency->load());
    pitchShifter.setOctaveStep(pPitchOctaveStep->load() > 0.5f);
    pitchShifter.process(buffer);

    chorusModule.setEnabled(pChorusEnabled->load() > 0.5f);
    chorusModule.setAmount(pChorusAmount->load());
    chorusModule.setRate(pChorusRate->load());
    chorusModule.process(buffer);

    // ── Cut Now: edge-detect rising edge → fade wet signal out, clear DSP, fade back in ──
    {
        const float cutNowVal = pCutNow->load();
        if (cutNowVal > 0.5f && lastCutNowValue <= 0.5f)
            cutNowState = CutNowState::FadingOut;
        lastCutNowValue = cutNowVal;

        if (cutNowState != CutNowState::Idle) {
            auto* const* chans = buffer.getArrayOfWritePointers();
            for (int i = 0; i < numSamples; ++i) {
                if (cutNowState == CutNowState::FadingOut) {
                    cutNowGain -= cutNowFadeOutRate;
                    if (cutNowGain <= 0.0f) {
                        cutNowGain = 0.0f;
                        fdnReverb.reset();
                        vibratoModule.reset();
                        pitchShifter.reset();
                        chorusModule.reset();
                        earlyReflections.reset();
                        predelayModule.reset();
                        cutNowState = CutNowState::FadingIn;
                    }
                } else {
                    cutNowGain += cutNowFadeInRate;
                    if (cutNowGain >= 1.0f) { cutNowGain = 1.0f; cutNowState = CutNowState::Idle; }
                }
                for (int ch = 0; ch < numChannels; ++ch)
                    chans[ch][i] *= cutNowGain;
            }
        }
    }

    // ===== PANEL 5: OUTPUT =====
    stereoWidener.setWidth(pStereo->load() / 120.0f);
    stereoWidener.process(buffer);

    dryWetMixer.setMix(pDryWet->load() / 100.0f);
    dryWetMixer.mixToOutput(buffer);
}

bool AudioPluginAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
