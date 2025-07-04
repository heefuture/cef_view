
#include "CefClientBrowser.h"

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "cef_control/util/util.h"

namespace cef
{
//////////////////////////////////////////////////////////////////////////////////////////
// CefBrowserProcessHandler methods.
class ClientBrowserDelegate : public ClientApp::BrowserDelegate {
public:
    ClientBrowserDelegate()
    {
        // Default schemes that support cookies.
        cookieable_schemes_.push_back("http");
        cookieable_schemes_.push_back("https");
    }

    virtual void OnContextInitialized(CefRefPtr<ClientApp> app) override
    {

        // Register cookieable schemes with the global cookie manager.
        CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager(NULL);
        ASSERT(manager.get());
        manager->SetSupportedSchemes(cookieableSchemes_, NULL);

        // 这里可以删除了保存的Cooies信息
        // manager->DeleteCookies(L"", L"", nullptr);
    }


    virtual void onBeforeChildProcessLaunch(CefRefPtr<ClientApp> app, CefRefPtr<CefCommandLine> command_line) override
    {
    }

    virtual void onRenderProcessThreadCreated(CefRefPtr<ClientApp> app, CefRefPtr<CefListValue> extra_info) override
    {
    }

private:
    // Schemes that will be registered with the global cookie manager.
    std::vector<CefString> cookieableSchemes_;
    IMPLEMENT_REFCOUNTING(ClientBrowserDelegate);
};

void CreateBrowserDelegatesInner(ClientApp::BrowserDelegateSet& delegates) {
  delegates.insert(new ClientBrowserDelegate);
}
}

