/**
 * @file        CefViewAppDelegateInterface.h
 * @brief       CefViewAppDelegateInterface class declaration
 * @version     1.0
 * @author      heefuture
 * @date        2025.09.04
 * @copyright
 */
#ifndef CEFVIEWAPPDELEGATEINTERFACE_H
#define CEFVIEWAPPDELEGATEINTERFACE_H
#pragma once
#include <memory>
#include <include/cef_app.h>

namespace cefview {

// Interface for browser delegates. All BrowserDelegates must be returned via
// CreateBrowserDelegates. Do not perform work in the BrowserDelegate
// constructor. See CefBrowserProcessHandler for documentation.
class CefViewAppDelegateInterface {
public:
    typedef std::shared_ptr<CefViewAppDelegateInterface> RefPtr;
    typedef std::weak_ptr<CefViewAppDelegateInterface> WeakPtr;
    virtual ~CefViewAppDelegateInterface() {}
#pragma region CefBrowserProcessHandler
    /////////////////////////////////////// CefBrowserProcessHandler methods: /////////////////////////////////////
    virtual void onContextInitialized() {}

    virtual void onBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> commandLine) {}

    virtual void onWebKitInitialized() {}
#pragma endregion // CefBrowserProcessHandler

#pragma region CefRenderProcessHandler
    /////////////////////////////////////// CefRenderProcessHandler methods: /////////////////////////////////////
    virtual void onBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extraInfo) {}

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
                                          CefProcessId sourceProcess,
                                          CefRefPtr<CefProcessMessage> message) { return false; }

    // Set a JavaScript callback for the specified |messageName| and |browserId|
    // combination. Will automatically be removed when the associated context is
    // released. Callbacks can also be set in JavaScript using the
    // app.setMessageCallback function.
    virtual void setMessageCallback(const std::string& messageName,
                                    int browserId,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Value> function) {}

    // Removes the JavaScript callback for the specified |messageName| and
    // |browserId| combination. Returns true if a callback was removed. Callbacks
    // can also be removed in JavaScript using the app.removeMessageCallback
    // function.
    virtual bool removeMessageCallback(const std::string& messageName, int browserId) { return false; }
#pragma endregion // CefRenderProcessHandler
};

}  // namespace cefview

#endif //!CEFVIEWAPPDELEGATEINTERFACE_H
