#pragma once

namespace CameraHelper
{
#if JUCE_MAC
    // Call after the WebBrowserComponent is visible and has started loading.
    // Finds the underlying WKWebView, enables mediaDevices, installs a
    // UIDelegate that auto-grants camera permission, then reloads the page.
    // peerHandle: result of webViewComponent->getPeer()->getNativeHandle()
    void enableCameraAccess(void* peerHandle);
#endif
}
