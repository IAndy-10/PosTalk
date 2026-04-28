#include "CameraHelper.h"

#if JUCE_MAC

#import <WebKit/WebKit.h>
#import <AVFoundation/AVFoundation.h>

// ---------------------------------------------------------------------------
// UIDelegate — auto-grants camera permission without showing a dialog
// ---------------------------------------------------------------------------
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

// Retained for the lifetime of the plugin
static JuceCameraDelegate* gCameraDelegate = nil;

// ---------------------------------------------------------------------------
// Recursively search an NSView hierarchy for the WKWebView instance
// ---------------------------------------------------------------------------
static WKWebView* findWKWebView(NSView* view)
{
    if ([view isKindOfClass:[WKWebView class]])
        return (WKWebView*)view;

    for (NSView* sub in view.subviews)
    {
        WKWebView* found = findWKWebView(sub);
        if (found) return found;
    }
    return nil;
}

// ---------------------------------------------------------------------------
namespace CameraHelper
{
    void enableCameraAccess(void* peerHandle)
    {
        // peerHandle is the NSView* that JUCE gave us via getPeer()->getNativeHandle()
        NSView* rootView = (__bridge NSView*)peerHandle;
        if (!rootView) return;

        WKWebView* wk = findWKWebView(rootView);
        if (!wk) return;

        // 1. Enable mediaDevices in WKPreferences (works after creation)
        //    Public key since macOS 12; use the underscore-prefixed private key for older OS.
        if (@available(macOS 12.0, *))
            [wk.configuration.preferences setValue:@YES forKey:@"mediaDevicesEnabled"];
        else
            [wk.configuration.preferences setValue:@YES forKey:@"_mediaDevicesEnabled"];

        // 2. Install the UIDelegate that auto-grants permission
        if (!gCameraDelegate)
            gCameraDelegate = [[JuceCameraDelegate alloc] init];
        wk.UIDelegate = gCameraDelegate;

        // 3. Request OS-level camera permission (triggers the macOS dialog once)
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                               completionHandler:^(BOOL granted)
        {
            if (granted)
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    // Reload so the page re-evaluates navigator.mediaDevices
                    // with the new preferences in effect
                    [wk reload];
                });
            }
        }];
    }
}

#endif // JUCE_MAC
