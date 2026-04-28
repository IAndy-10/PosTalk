#pragma once

#include <JuceHeader.h>
#include <functional>

// Bridge between the WebView (TypeScript/Svelte UI) and C++ plugin code.
// JS → C++: Intercepts juce:// URLs for parameter changes.
// C++ → JS: Uses evaluateJavascript() to call window.setParameterValue().
class WebViewBridge : public juce::WebBrowserComponent {
public:
    WebViewBridge() = default;

    using ParameterCallback = std::function<void(const juce::String&, float)>;

    void setParameterCallback(ParameterCallback callback) {
        parameterCallback = std::move(callback);
    }

    bool pageAboutToLoad(const juce::String& newURL) override {
        if (newURL.startsWith("juce://")) {
            handleBridgeURL(newURL);
            return false;
        }
        return true;
    }

private:
    ParameterCallback parameterCallback;

    void handleBridgeURL(const juce::String& urlStr) {
        if (!urlStr.contains("setparameter")) return;

        juce::URL juceURL(urlStr);
        auto paramNames  = juceURL.getParameterNames();
        auto paramValues = juceURL.getParameterValues();

        int nameIdx  = paramNames.indexOf("name");
        int valueIdx = paramNames.indexOf("value");

        if (nameIdx >= 0 && valueIdx >= 0 && parameterCallback) {
            juce::String paramName = paramValues[nameIdx];
            float paramValue       = paramValues[valueIdx].getFloatValue();
            parameterCallback(paramName, paramValue);
        }
    }
};
