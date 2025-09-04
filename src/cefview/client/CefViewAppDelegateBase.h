/**
* @file        CefViewAppDelegateBase.h
* @brief       CefViewAppDelegateBase class declaration
* @version     1.0
* @author      heefuture
* @date        2025.09.04
* @copyright
*/
#ifndef CEFVIEWAPPDELEGATEBASE_H
#define CEFVIEWAPPDELEGATEBASE_H
#pragma once
#include <memory>
#include <include/cef_app.h>

namespace cefview {

// Interface for browser delegates. All BrowserDelegates must be returned via
// CreateBrowserDelegates. Do not perform work in the BrowserDelegate
// constructor. See CefBrowserProcessHandler for documentation.
class CefViewAppDelegateBase {
public:
    typedef std::shared_ptr<CefViewAppDelegateBase> RefPtr;
    typedef std::weak_ptr<CefViewAppDelegateBase> WeakPtr;
    virtual ~CefViewAppDelegateBase() {}
#pragma region CefBrowserProcessHandler
    /////////////////////////////////////// CefBrowserProcessHandler methods: /////////////////////////////////////
    virtual void onContextInitialized() {}

    virtual void onBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) {}

    virtual void onWebKitInitialized() {}
#pragma endregion // CefBrowserProcessHandler

#pragma region CefRenderProcessHandler
    /////////////////////////////////////// CefRenderProcessHandler methods: /////////////////////////////////////
    virtual void onBrowserCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefDictionaryValue> extra_info) {}

    virtual void onBrowserDestroyed(CefRefPtr<CefBrowser> browser) {}

    virtual void onContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) {}


    virtual void onContextReleased(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefV8Context> context) {}

    virtual void onUncaughtException(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context,
                                     CefRefPtr<CefV8Exception> exception,
                                     CefRefPtr<CefV8StackTrace> stackTrace) {}

    virtual void onFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefDOMNode> node) {}

    // Called when a process message is received. Return true if the message was
    // handled and should not be passed on to other handlers. RenderDelegates
    // should check for unique message names to avoid interfering with each
    // other.
    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) { return false; }

    // Set a JavaScript callback for the specified |message_name| and |browser_id|
    // combination. Will automatically be removed when the associated context is
    // released. Callbacks can also be set in JavaScript using the
    // app.setMessageCallback function.
    virtual void setMessageCallback(const std::string &message_name,
                                    int browser_id,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Value> function) {}

    // Removes the JavaScript callback for the specified |message_name| and
    // |browser_id| combination. Returns true if a callback was removed. Callbacks
    // can also be removed in JavaScript using the app.removeMessageCallback
    // function.
    virtual bool removeMessageCallback(const std::string &message_name, int browser_id) { return false; }
#pragma endregion // CefRenderProcessHandler
};

}  // namespace cefview

#endif //!CEFVIEWAPPDELEGATEBASE_H