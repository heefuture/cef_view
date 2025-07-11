#include "CefClient.h"
#include <string>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"

#include <util/util.h>
#include <manager/cefManager.h>
#include <client/CefSwitches.h>
#include <client/CefClientRender.h>
#include <client/CefClientBrowser.h>

namespace cef
{

ClientApp::ClientApp()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefApp methods.
void ClientApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
    command_line->AppendSwitch(cef::kNoProxyServer);
    command_line->AppendSwitch(cef::kWinHttpProxyResolver);

#ifdef _CEF_DEBUG
    CefString tempDir = AppGetTempDirectory();
    if (!tempDir.empty()) {
        command_line->AppendSwitchWithValue(cefclient::kUncaughtExceptionStackSize, "2");
        command_line->AppendSwitchWithValue(cefclient::kLogSeverity, cefclient::log_severity::kVerbose);
        command_line->AppendSwitchWithValue(cefclient::kLogFile, AppGetCefDebugLogFileName());
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefBrowserProcessHandler methods.
void ClientApp::OnContextInitialized()
{
    BrowserDelegateSet::iterator it = browser_delegates_.begin();
    for (; it != browser_delegates_.end(); ++it)
        (*it)->onContextInitialized(this);
}

void ClientApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
{
    BrowserDelegateSet::iterator it = browser_delegates_.begin();
    for (; it != browser_delegates_.end(); ++it)
        (*it)->onBeforeChildProcessLaunch(this, command_line);
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefRenderProcessHandler methods.
void ClientApp::OnWebKitInitialized()
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->onWebKitInitialized(this);
}

void ClientApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->onBrowserCreated(this, browser, extra_info);
}

void ClientApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->onBrowserDestroyed(this, browser);
}

void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->onContextCreated(this, browser, frame, context);
}

void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,	CefRefPtr<CefV8Context> context)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->onContextReleased(this, browser, frame, context);

}

void ClientApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    (*it)->onUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientApp::OnFocusedNodeChanged(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefDOMNode> node)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->onFocusedNodeChanged(this, browser, frame, node);
}

bool ClientApp::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    assert(source_process == PID_BROWSER);

    bool handled = false;
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end() && !handled; ++it) {
        handled = (*it)->onProcessMessageReceived(this, browser, source_process, message);
    }

    return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// static
void ClientApp::CreateBrowserDelegates(BrowserDelegateSet& delegates) {
    CreateBrowserDelegatesInner(delegates);
}

// static
void ClientApp::CreateRenderDelegates(RenderDelegateSet& delegates) {
    CreateRenderDelegatesInner(delegates);
}

}