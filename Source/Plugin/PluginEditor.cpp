// Apple frameworks MUST be imported before JUCE headers to avoid the
// juce::Point template shadowing Carbon's plain `Point` struct (Finder.h:248).
#if defined(__APPLE__)
#import <WebKit/WebKit.h>
#import <AVFoundation/AVFoundation.h>
#endif

#include "PluginEditor.h"
#include "ParameterIDs.h"

// ── macOS camera setup (compiled as Obj-C++ via CMake COMPILE_FLAGS) ────────
#if JUCE_MAC

// UIDelegate: auto-grants camera permission without showing a WebKit dialog
@interface JuceCameraDelegate : NSObject <WKUIDelegate>
@end
@implementation JuceCameraDelegate
- (void)webView:(WKWebView*)webView
    requestMediaCapturePermissionForOrigin:(WKSecurityOrigin*)origin
    initiatedByFrame:(WKFrameInfo*)frame
    type:(WKMediaCaptureType)type
    decisionHandler:(void (^)(WKPermissionDecision))decisionHandler
    API_AVAILABLE(macos(12.0))
{
    decisionHandler(WKPermissionDecisionGrant);
}
@end

static JuceCameraDelegate* gCameraDelegate = nil;

static WKWebView* findWKWebView(NSView* view)
{
    if ([view isKindOfClass:[WKWebView class]])
        return (WKWebView*)view;
    for (NSView* sub in view.subviews)
        if (auto* found = findWKWebView(sub)) return found;
    return nil;
}

static void enableCameraInWebView(NSView* rootView)
{
    WKWebView* wk = findWKWebView(rootView);
    if (!wk) return;

    // Enable mediaDevices for file:// origins
    if (@available(macOS 12.0, *))
        [wk.configuration.preferences setValue:@YES forKey:@"mediaDevicesEnabled"];
    else
        [wk.configuration.preferences setValue:@YES forKey:@"_mediaDevicesEnabled"];

    if (!gCameraDelegate)
        gCameraDelegate = [[JuceCameraDelegate alloc] init];
    wk.UIDelegate = gCameraDelegate;

    // Trigger the one-time macOS camera permission dialog, then reload
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                           completionHandler:^(BOOL granted) {
        if (granted)
            dispatch_async(dispatch_get_main_queue(), ^{ [wk reload]; });
    }];
}
#endif
// ────────────────────────────────────────────────────────────────────────────

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    webView = std::make_unique<WebViewBridge>();
    addAndMakeVisible(webView.get());

    webView->setParameterCallback([this](const juce::String& paramId, float value) {
        onParameterChangedFromJS(paramId, value);
    });

    setSize(1260, 700);
    setResizable(true, true);
    setResizeLimits(450, 280, 1800, 1120);

    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("plugin_camera_v1_" +
                                     juce::String(juce::Time::currentTimeMillis()));
    tempDir.createDirectory();

    auto htmlFile = tempDir.getChildFile("index.html");
    htmlFile.replaceWithData(BinaryData::index_html, BinaryData::index_htmlSize);

    webView->goToURL("file://" + htmlFile.getFullPathName());

    startTimerHz(30);
    sendAllParamsToJS();

#if JUCE_MAC
    // 300 ms: enough time for WKWebView to be added to the NSView hierarchy
    juce::Timer::callAfterDelay(300, [this]() {
        if (auto* peer = getPeer())
            enableCameraInWebView((NSView*)peer->getNativeHandle());
    });
#endif
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xffede6da));
}

void AudioPluginAudioProcessorEditor::resized() {
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}

void AudioPluginAudioProcessorEditor::timerCallback() {
    auto& apvts = processorRef.apvts;

    auto sendIfChanged = [&](const juce::String& paramId) {
        auto* param = apvts.getParameter(paramId);
        if (!param) return;
        float normalized = param->getValue();
        auto key = paramId.toStdString();
        auto it = lastParamValues.find(key);
        if (it == lastParamValues.end() || std::abs(it->second - normalized) > 0.001f) {
            lastParamValues[key] = normalized;
            sendParamToJS(paramId, normalized);
        }
    };

    using namespace PluginParamIDs;
    sendIfChanged(loCutEnabled.getParamID()); sendIfChanged(hiCutEnabled.getParamID());
    sendIfChanged(loCutFreq.getParamID());    sendIfChanged(hiCutFreq.getParamID());
    sendIfChanged(erEnabled.getParamID());    sendIfChanged(erAmount.getParamID());
    sendIfChanged(erRate.getParamID());       sendIfChanged(erShape.getParamID());
    sendIfChanged(reverbMode.getParamID());   sendIfChanged(crossoverFreq.getParamID());
    sendIfChanged(diffusion.getParamID());    sendIfChanged(scale.getParamID());
    sendIfChanged(decay.getParamID());        sendIfChanged(damping.getParamID());
    sendIfChanged(feedback.getParamID());
    sendIfChanged(chorusAmount.getParamID()); sendIfChanged(chorusRate.getParamID());
    sendIfChanged(reflectGain.getParamID());  sendIfChanged(diffuseGain.getParamID());
    sendIfChanged(dryWet.getParamID());
    sendIfChanged(predelay.getParamID());     sendIfChanged(smooth.getParamID());
    sendIfChanged(size.getParamID());         sendIfChanged(freeze.getParamID());
    sendIfChanged(flatEnabled.getParamID());  sendIfChanged(cutEnabled.getParamID());
    sendIfChanged(stereo.getParamID());       sendIfChanged(highFilterType.getParamID());
    sendIfChanged(density.getParamID());
    sendIfChanged(gainDb.getParamID());       sendIfChanged(outputTrimDb.getParamID());
    sendIfChanged(satDrive.getParamID());     sendIfChanged(satMix.getParamID());
    sendIfChanged(satTone.getParamID());
    sendIfChanged(timbreCutoff.getParamID());    sendIfChanged(timbreResonance.getParamID());
    sendIfChanged(timbreDrivePre.getParamID());
    sendIfChanged(vibratoRate.getParamID());     sendIfChanged(vibratoDepth.getParamID());
    sendIfChanged(vibratoFadeIn.getParamID());
}

void AudioPluginAudioProcessorEditor::sendAllParamsToJS() {
    auto& apvts = processorRef.apvts;
    auto sendAll = [&](const juce::String& paramId) {
        auto* param = apvts.getParameter(paramId);
        if (!param) return;
        float normalized = param->getValue();
        lastParamValues[paramId.toStdString()] = normalized;
        sendParamToJS(paramId, normalized);
    };

    using namespace PluginParamIDs;
    sendAll(loCutEnabled.getParamID()); sendAll(hiCutEnabled.getParamID());
    sendAll(loCutFreq.getParamID());    sendAll(hiCutFreq.getParamID());
    sendAll(erEnabled.getParamID());    sendAll(erAmount.getParamID());
    sendAll(erRate.getParamID());       sendAll(erShape.getParamID());
    sendAll(reverbMode.getParamID());   sendAll(crossoverFreq.getParamID());
    sendAll(diffusion.getParamID());    sendAll(scale.getParamID());
    sendAll(decay.getParamID());        sendAll(damping.getParamID());
    sendAll(feedback.getParamID());
    sendAll(chorusAmount.getParamID()); sendAll(chorusRate.getParamID());
    sendAll(reflectGain.getParamID());  sendAll(diffuseGain.getParamID());
    sendAll(dryWet.getParamID());
    sendAll(predelay.getParamID());     sendAll(smooth.getParamID());
    sendAll(size.getParamID());         sendAll(freeze.getParamID());
    sendAll(flatEnabled.getParamID());  sendAll(cutEnabled.getParamID());
    sendAll(stereo.getParamID());       sendAll(highFilterType.getParamID());
    sendAll(density.getParamID());
    sendAll(gainDb.getParamID());       sendAll(outputTrimDb.getParamID());
    sendAll(satDrive.getParamID());     sendAll(satMix.getParamID());
    sendAll(satTone.getParamID());
    sendAll(timbreCutoff.getParamID());    sendAll(timbreResonance.getParamID());
    sendAll(timbreDrivePre.getParamID());
    sendAll(vibratoRate.getParamID());     sendAll(vibratoDepth.getParamID());
    sendAll(vibratoFadeIn.getParamID());
}

void AudioPluginAudioProcessorEditor::sendParamToJS(const juce::String& paramId,
                                                     float normalizedValue) {
    juce::String js = "if (window.setParameterValue) { window.setParameterValue('"
                      + paramId + "', " + juce::String(normalizedValue) + "); }";
    webView->evaluateJavascript(js);
}

void AudioPluginAudioProcessorEditor::onParameterChangedFromJS(const juce::String& paramId,
                                                                float value) {
    auto* param = processorRef.apvts.getParameter(paramId);
    if (param != nullptr) {
        param->setValueNotifyingHost(value);
        lastParamValues[paramId.toStdString()] = value;
    }
}
