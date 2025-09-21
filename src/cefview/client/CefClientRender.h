#ifndef CEFCLIENTRENDER_H
#define CEFCLIENTRENDER_H
#pragma once

#include "include/cef_base.h"
#include <client/CefViewApp.h>

namespace cefview {

// Message sent when the focused node changes.
extern const char kFocusedNodeChangedMessage[];

// // Create the render delegate.
// void CreateRenderDelegatesInner(CefViewApp::RenderDelegateSet& delegates);

}  // namespace cefview

#endif //!CEFCLIENTRENDER_H