#include "stdafx.h"
#include "client_app.h"
#include <string>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "cef_control/manager/cef_manager.h"
#include "cef_control/util/util.h"

namespace cef
{

ClientApp::ClientApp()
{
    last_node_is_editable_ = false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefApp methods.
void ClientApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
    command_line->AppendSwitch(cefclient::kNoProxyServer);
    command_line->AppendSwitch(cefclient::kWinHttpProxyResolver);

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
    // Register cookieable schemes with the global cookie manager.
    CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager(NULL);
    ASSERT(manager.get());
    manager->SetSupportedSchemes(cookieable_schemes_, NULL);

    // 这里可以删除了保存的Cooies信息
    // manager->DeleteCookies(L"", L"", nullptr);
}

void ClientApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// CefRenderProcessHandler methods.

void ClientApp::OnWebKitInitialized()
{
    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();
    std::cout << "ClientApp::OnWebKitInitialized (Process id " << pid << "), (Thread id " << tid << ")" << std::endl;

    // Register the client_app extension.
    CefRefPtr<ClientAppExtensionHandler> handler(new ClientAppExtensionHandler(this));
    std::string app_code =
        "var app;"
        "if (!app)"
        "  app = {};"
        "(function() {"
        "  app.sendMessage = function(name, arguments) {"
        "    native function sendMessage();"
        "    return sendMessage(name, arguments);"
        "  };"
        "  app.setMessageCallback = function(name, callback) {"
        "    native function setMessageCallback();"
        "    return setMessageCallback(name, callback);"
        "  };"
        "  app.removeMessageCallback = function(name) {"
        "    native function removeMessageCallback();"
        "    return removeMessageCallback(name);"
        "  };"
        "})();";
    CefRegisterExtension("v8/app", app_code, handler.get());


    CefRefPtr<CefJSHandler> handler = new CefJSHandler();

    if (!render_js_bridge_.get())
        render_js_bridge_.reset(new CefJSBridge);
    handler->AttachJSBridge(render_js_bridge_);
     CefRegisterExtension("v8/extern", extensionCode, handler);
}

void ClientApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info)
{
    if (!render_js_bridge_.get())
        render_js_bridge_.reset(new CefJSBridge);
}

void ClientApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
}

void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->OnContextCreated(this, browser, frame, context);
}

void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,	CefRefPtr<CefV8Context> context)
{
    RenderDelegateSet::iterator it = render_delegates_.begin();
    for (; it != render_delegates_.end(); ++it)
        (*it)->OnContextReleased(this, browser, frame, context);

}

void ClientApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
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
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    ASSERT(source_process == PID_BROWSER);

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