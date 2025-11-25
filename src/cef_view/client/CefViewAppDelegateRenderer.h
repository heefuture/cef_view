/**
* @file        CefViewAppDelegateRenderer.h
* @brief       
* @version     1.0
* @author      heefuture
* @date        2025.11.21
* @copyright
*/
#ifndef CEFVIEWAPPDELEGATERENDERER_H
#define CEFVIEWAPPDELEGATERENDERER_H
#pragma once


#include "include/cef_base.h"

#include <client/CefViewAppDelegateInterface.h>

namespace cefview {

class CefJsBridgeRender;

class CefViewAppDelegateRenderer : public CefViewAppDelegateInterface {
public:
   CefViewAppDelegateRenderer()
     : _lastNodeIsEditable(false) {
   }

   virtual void onWebKitInitialized() override;
  
   virtual void onBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extraInfo) override;

   virtual void onContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;

   virtual void onFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) override;

   virtual void onUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) override;

   virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId sourceProcess, CefRefPtr<CefProcessMessage> message) override;
private:
   bool _lastNodeIsEditable{false};
   std::shared_ptr<CefJsBridgeRender> _renderJsBridge;
};

}  // namespace cefview

#endif //!CEFVIEWAPPDELEGATERENDERER_H