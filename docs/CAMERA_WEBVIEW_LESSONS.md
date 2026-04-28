# Camera + WKWebView in JUCE — Lessons Learned

## 1. Separate `.mm` files don't link in JUCE's SharedCode architecture

Adding a `.mm` file via `target_sources` inside an `if(APPLE)` block causes
`Undefined symbols` linker errors. JUCE's internal `SharedCode` static library
does not reliably pick up conditionally-added sources.

**Fix:** compile `PluginEditor.cpp` itself as Objective-C++:

```cmake
if(APPLE)
    set_source_files_properties(Source/Plugin/PluginEditor.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()
```

Then put all Obj-C++ code directly inside `PluginEditor.cpp` under `#if JUCE_MAC`.

---

## 2. `navigator.mediaDevices` is undefined on `file://` URLs in WKWebView

WKWebView does not consider `file://` a secure context, so WebKit strips
`navigator.mediaDevices` entirely — `getUserMedia` is unreachable from JS.

**Fix:** configure WKWebView from C++ after the page loads:

```objc
// 1. Enable mediaDevices in WKPreferences (works post-creation)
if (@available(macOS 12.0, *))
    [wk.configuration.preferences setValue:@YES forKey:@"mediaDevicesEnabled"];
else
    [wk.configuration.preferences setValue:@YES forKey:@"_mediaDevicesEnabled"];

// 2. Auto-grant camera permission via WKUIDelegate
wk.UIDelegate = gCameraDelegate; // see JuceCameraDelegate below

// 3. Request OS permission (one-time dialog), then reload
[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                       completionHandler:^(BOOL granted) {
    if (granted)
        dispatch_async(dispatch_get_main_queue(), ^{ [wk reload]; });
}];
```

```objc
@interface JuceCameraDelegate : NSObject <WKUIDelegate> @end
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
```

Call the setup 300 ms after `goToURL()` (WKWebView needs time to attach):

```cpp
juce::Timer::callAfterDelay(300, [this]() {
    if (auto* peer = getPeer())
        enableCameraInWebView((NSView*)peer->getNativeHandle());
});
```

---

## 3. Apple `#import` must come BEFORE JUCE headers

JUCE's `Point<T>` template shadows Carbon's plain `Point` struct.
If `<WebKit/WebKit.h>` is imported after JUCE headers, the compiler hits:

```
CarbonCore/Finder.h:248: error: use of class template 'Point' requires
template arguments; argument deduction not allowed in non-static struct member
```

**Fix:** put Apple imports at the very top of the file:

```cpp
// ── TOP OF FILE — before any #include ──────────────────────────────
#if defined(__APPLE__)
#import <WebKit/WebKit.h>
#import <AVFoundation/AVFoundation.h>
#endif

#include "PluginEditor.h"   // JUCE headers come after
#include "ParameterIDs.h"
```

---

## 4. CMake / Info.plist requirements

```cmake
# NSCameraUsageDescription — required by macOS; without it the OS silently
# denies the permission dialog.
juce_add_plugin(AudioPlugin02
    ...
    PLIST_TO_MERGE "<plist><dict><key>NSCameraUsageDescription</key><string>This plugin displays the camera feed for visual feedback.</string></dict></plist>"
)

# Link Apple frameworks
if(APPLE)
    target_link_libraries(AudioPlugin02 PRIVATE
        "-framework AVFoundation"
        "-framework WebKit"
    )
endif()
```

---

## Next: MediaPipe integration

Possible approaches for running MediaPipe inside this plugin:

| Approach | Runs where | Notes |
|---|---|---|
| **MediaPipe WASM** | Browser (JS) | Easiest — load `@mediapipe/tasks-vision` in Svelte, feed the `<video>` element directly |
| **MediaPipe C++ native** | C++ thread | Most powerful — process frames in the audio thread's companion thread, send results to JS |
| **MediaPipe via Python server** | Separate process | Good for prototyping; plugin connects via WebSocket |

The **WASM approach** is the fastest to get running since `navigator.mediaDevices`
is now available in the WKWebView — MediaPipe can consume the same video stream.
