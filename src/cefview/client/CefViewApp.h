/**
* @file        CefViewApp.h
* @brief       CefViewApp class declaration
* @version     1.0
* @author      heefuture
* @date        2025.09.04
* @copyright
*/
#ifndef CEFVIEWAPP_H
#define CEFVIEWAPP_H
#pragma once

#include <set>
#include <string>
#include <utility>
#include <vector>
#include "include/cef_app.h"
#include "include/cef_base.h"

#include "CefViewAppDelegateBase.h"

namespace cefview
{

class CefViewApp : public CefApp,
                   public CefBrowserProcessHandler,
                   public CefRenderProcessHandler
{
public:

    typedef std::set<CefViewAppDelegateBase::WeakPtr> ViewAppDelegateSet;

public:
    CefViewApp();
    ~CefViewApp();
private:
#pragma region CefApp
    /////////////////////////////////////// CefApp methods: /////////////////////////////////////
    virtual void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) override;

    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }
#pragma endregion CefApp

#pragma region CefBrowserProcessHandler
    /////////////////////////////////////// CefBrowserProcessHandler methods: /////////////////////////////////////
    virtual void OnContextInitialized() override;

    virtual void OnBeforeChildProcessLaunch( CefRefPtr<CefCommandLine> command_line) override;
#pragma endregion CefBrowserProcessHandler

#pragma region CefRenderProcessHandler
    /////////////////////////////////////// CefRenderProcessHandler methods: /////////////////////////////////////
    virtual void OnWebKitInitialized() override;

    virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) override;

    virtual void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;

    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;

    virtual void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;

    virtual void OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context,
                                     CefRefPtr<CefV8Exception> exception,
                                     CefRefPtr<CefV8StackTrace> stackTrace) override;

    virtual void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) override;

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) override;
#pragma endregion CefRenderProcessHandler

    // Creates all of the ViewDelegate objects. Implemented in
    // client_app_delegates.
    static void CreateViewAppDelegates(ViewAppDelegateSet& delegates);

    // Creates all of the RenderDelegate objects. Implemented in
    // client_app_delegates.
    // static void CreateRenderDelegates(RenderDelegateSet& delegates);

private:
    ViewAppDelegateSet _viewAppDelegates;

    IMPLEMENT_REFCOUNTING(CefViewApp);
};

}

#endif //!CEFVIEWAPP_H