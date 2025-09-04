
#include "CefClientBrowser.h"

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"


namespace cefview
{
//////////////////////////////////////////////////////////////////////////////////////////
// CefBrowserProcessHandler methods.
class ClientBrowserDelegate : public ClientApp::BrowserDelegate {
public:
    ClientBrowserDelegate()
    {
    }

    virtual void onContextInitialized(CefRefPtr<ClientApp> app) override
    {
    }


    virtual void onBeforeChildProcessLaunch(CefRefPtr<ClientApp> app, CefRefPtr<CefCommandLine> command_line) override
    {
    }
private:
    IMPLEMENT_REFCOUNTING(ClientBrowserDelegate);
};

void CreateBrowserDelegatesInner(ClientApp::BrowserDelegateSet& delegates) {
  delegates.insert(new ClientBrowserDelegate);
}
}

