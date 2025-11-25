#include "CefViewApp.h"
#include <string>

#include <include/cef_cookie.h>
#include <include/cef_process_message.h>
#include <include/cef_task.h>
#include <include/cef_v8.h>

#include <utils/util.h>
#include <client/CefSwitches.h>
#include <client/CefClientRender.h>
#include <client/CefClientBrowser.h>

namespace cefview
{

CefViewApp::CefViewApp()
{
}

CefViewApp::CefViewApp(CefViewAppDelegateInterface::RefPtr delegate)
{
    if (delegate) {
        _viewAppDelegates.insert(delegate);
    }
}

CefViewApp::~CefViewApp()
{
    _viewAppDelegates.clear();
}

void CefViewApp::RegisterDelegate(CefViewAppDelegateInterface::RefPtr delegate)
{
    if (delegate) {
        _viewAppDelegates.insert(delegate);
    }
}

void CefViewApp::UnregisterDelegate(CefViewAppDelegateInterface::RefPtr delegate)
{
    if (delegate) {
        _viewAppDelegates.erase(delegate);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////
// CefApp methods.
void CefViewApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
    command_line->AppendSwitch(cefview::kNoProxyServer);
    command_line->AppendSwitch(cefview::kWinHttpProxyResolver);

#ifdef _CEF_DEBUG
    CefString tempDir = AppGetTempDirectory();
    if (!tempDir.empty()) {
        command_line->AppendSwitchWithValue(cefview::kUncaughtExceptionStackSize, "2");
        command_line->AppendSwitchWithValue(cefview::kLogSeverity, cefview::log_severity::kVerbose);
        command_line->AppendSwitchWithValue(cefview::kLogFile, AppGetCefDebugLogFileName());
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefBrowserProcessHandler methods.
void CefViewApp::OnContextInitialized()
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onContextInitialized();
        }
    }
}

void CefViewApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onBeforeChildProcessLaunch(command_line);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefRenderProcessHandler methods.
void CefViewApp::OnWebKitInitialized()
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onWebKitInitialized();
        }
    }
}

void CefViewApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onBrowserCreated(browser, extra_info);
        }
    }
}

void CefViewApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onBrowserDestroyed(browser);
        }
    }
}

void CefViewApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onContextCreated(browser, frame, context);
        }
    }
}

void CefViewApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onContextReleased(browser, frame, context);
        }
    }
}

void CefViewApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefV8Context> context,
                                     CefRefPtr<CefV8Exception> exception,
                                     CefRefPtr<CefV8StackTrace> stackTrace)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
        delegate->onUncaughtException(browser, frame, context, exception, stackTrace);
        }
    }
}

void CefViewApp::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node)
{
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            delegate->onFocusedNodeChanged(browser, frame, node);
        }
    }
}

bool CefViewApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message)
{
    assert(source_process == PID_BROWSER);

    bool handled = false;
    for (auto& weakDelegate : _viewAppDelegates) {
        if (auto delegate = weakDelegate.lock()) {
            handled = delegate->onProcessMessageReceived(browser, source_process, message);
        }
    }

    return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// // static
// void CefViewApp::CreateViewAppDelegates(ViewAppDelegateSet& delegates) {
//    CreateBrowserDelegatesInner(delegates);
// }

// // static
// void CefViewApp::CreateRenderDelegates(ViewAppDelegateSet& delegates) {
//     CreateRenderDelegatesInner(delegates);
// }

}