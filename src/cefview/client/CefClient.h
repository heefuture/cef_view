/** @file client_app.h
 * @brief 定义Browser进程的CefApp类，管理Cef模块的生命周期
 * @copyright (c) 2016, NetEase Inc. All rights reserved
 * @author Redrain
 * @date 2016/7/19
 */
#pragma once

#include <set>
#include <string>
#include <utility>
#include <vector>
#include "include/cef_app.h"
#include "webview/client/cef_js_bridge.h"

namespace cef
{

class ClientApp : public CefApp,
                      public CefBrowserProcessHandler,
                      public CefRenderProcessHandler
{
public:
    // Interface for browser delegates. All BrowserDelegates must be returned via
    // CreateBrowserDelegates. Do not perform work in the BrowserDelegate
    // constructor. See CefBrowserProcessHandler for documentation.
    class BrowserDelegate : public virtual CefBase {
    public:
        virtual void onContextInitialized(CefRefPtr<ClientApp> app) {}

        virtual void onBeforeChildProcessLaunch(
            CefRefPtr<ClientApp> app,
            CefRefPtr<CefCommandLine> command_line) {}

        virtual void onRenderProcessThreadCreated(
            CefRefPtr<ClientApp> app,
            CefRefPtr<CefListValue> extra_info) {}

        virtual void onRenderProcessThreadCreated(
            CefRefPtr<ClientApp> app,
            CefRefPtr<CefListValue> extra_info) {}
    };
    typedef std::set<CefRefPtr<BrowserDelegate> > BrowserDelegateSet;

    // Interface for renderer delegates. All RenderDelegates must be returned via
    // CreateRenderDelegates. Do not perform work in the RenderDelegate
    // constructor. See CefRenderProcessHandler for documentation.
    class RenderDelegate : public virtual CefBase {
    public:
        virtual void onRenderThreadCreated(CefRefPtr<ClientApp> app,
            CefRefPtr<CefListValue> extra_info) {}

        virtual void onWebKitInitialized(CefRefPtr<ClientApp> app) {}

        virtual void onBrowserCreated(CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefDictionaryValue> extra_info) {}

        virtual void onBrowserDestroyed(CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser) {}

        virtual void onContextCreated(CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefV8Context> context) {}

        virtual void onContextReleased(CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefV8Context> context) {}

        virtual void onUncaughtException(CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefV8Context> context,
            CefRefPtr<CefV8Exception> exception,
            CefRefPtr<CefV8StackTrace> stackTrace) {}

        virtual void onFocusedNodeChanged(CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefDOMNode> node) {}

        // Called when a process message is received. Return true if the message was
        // handled and should not be passed on to other handlers. RenderDelegates
        // should check for unique message names to avoid interfering with each
        // other.
        virtual bool onProcessMessageReceived(
            CefRefPtr<ClientApp> app,
            CefRefPtr<CefBrowser> browser,
            CefProcessId source_process,
            CefRefPtr<CefProcessMessage> message) {
                return false;
        }

        typedef std::set<CefRefPtr<RenderDelegate>> RenderDelegateSet;
public:
    ClientApp();

    // Set a JavaScript callback for the specified |message_name| and |browser_id|
    // combination. Will automatically be removed when the associated context is
    // released. Callbacks can also be set in JavaScript using the
    // app.setMessageCallback function.
    void SetMessageCallback(const std::string &message_name,
        int browser_id,
        CefRefPtr<CefV8Context> context,
        CefRefPtr<CefV8Value> function);

    // Removes the JavaScript callback for the specified |message_name| and
    // |browser_id| combination. Returns true if a callback was removed. Callbacks
    // can also be removed in JavaScript using the app.removeMessageCallback
    // function.
    bool RemoveMessageCallback(const std::string &message_name, int browser_id);

protected:
    // CefApp methods.
    virtual void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) override;

private:
    // Creates all of the BrowserDelegate objects. Implemented in
    // client_app_delegates.
    static void CreateBrowserDelegates(BrowserDelegateSet& delegates);

    // Creates all of the RenderDelegate objects. Implemented in
    // client_app_delegates.
    static void CreateRenderDelegates(RenderDelegateSet& delegates);

    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

    // CefBrowserProcessHandler methods.
    virtual void OnContextInitialized() override;
    virtual void OnBeforeChildProcessLaunch( CefRefPtr<CefCommandLine> command_line) override;
    // CefRenderProcessHandler methods.
    virtual void OnWebKitInitialized() override;
    virtual void OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info) override;
    virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) override;
    virtual void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;
    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override;
    virtual void OnContextReleased(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override;
    virtual void OnUncaughtException(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context,
        CefRefPtr<CefV8Exception> exception,
        CefRefPtr<CefV8StackTrace> stackTrace) override;
    virtual void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefDOMNode> node) override;
    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

    // Set of supported BrowserDelegates.
    BrowserDelegateSet browser_delegates_;

    // Set of supported RenderDelegates.
    RenderDelegateSet render_delegates_;

    IMPLEMENT_REFCOUNTING(ClientApp);
};

}