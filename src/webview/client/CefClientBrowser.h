#ifndef CEFCLIENTBROWSER_H
#define CEFCLIENTBROWSER_H
#pragma once

#include "include/cef_base.h"
#include "CefClient.h"

namespace cef {

// Message sent when the focused node changes.
extern const char kFocusedNodeChangedMessage[];

// Create the render delegate.
void CreateBrowserDelegatesInner(ClientApp::BrowserDelegateSet& delegates);

}  // namespace client_renderer

#endif //!CEFCLIENTBROWSER_H