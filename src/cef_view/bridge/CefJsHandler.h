
#pragma once
#include "include/cef_base.h"
#include "include/cef_app.h"

#include <client/CefViewApp.h>

namespace cefview
{
class CefJsBridgeRender;

class CefJSHandler : public CefV8Handler
{
public:
   CefJSHandler() {}
   virtual bool Execute(const CefString& name,
                        CefRefPtr<CefV8Value> object,
                        const CefV8ValueList& arguments,
                        CefRefPtr<CefV8Value>& retval,
                        CefString& exception) override;

   void registerJsBridge(std::shared_ptr<CefJsBridgeRender> jsBridge) { _jsBridge = jsBridge; }

   IMPLEMENT_REFCOUNTING(CefJSHandler);
protected:
   std::shared_ptr<CefJsBridgeRender> _jsBridge;
};

}

