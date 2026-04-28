#pragma once

#include "PluginProcessor.h"
#include "WebviewBridge.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <unordered_map>
#include <string>

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer {
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AudioPluginAudioProcessor& processorRef;
    std::unique_ptr<WebViewBridge> webView;

    // Handle parameter change from JS → C++
    void onParameterChangedFromJS(const juce::String& paramId, float value);

    // Send all parameters to JS (on init or timer)
    void sendAllParamsToJS();
    void sendParamToJS(const juce::String& paramId, float normalizedValue);

    // Snapshot of last sent values to detect changes
    std::unordered_map<std::string, float> lastParamValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
