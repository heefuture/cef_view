#ifndef CEFCLIENTRENDER_H
#define CEFCLIENTRENDER_H
#pragma once

#include "include/cef_base.h"
#include "CefClient.h"

namespace cef {

// Message sent when the focused node changes.
extern const char kFocusedNodeChangedMessage[];

// Create the render delegate.
void CreateRenderDelegatesInner(ClientApp::RenderDelegateSet& delegates);

}  // namespace client_renderer

#endif //!CEFCLIENTRENDER_H