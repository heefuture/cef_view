#pragma once
#include "include/cef_base.h"
#include "include/cef_app.h"

#include <client/CefViewApp.h>

namespace cefview {

class CefJsBridgeRender;

/// CefJSHandler handles JavaScript function calls from the renderer process.
/// It acts as a bridge between JavaScript code and native C++ functionality,
/// delegating actual execution to the registered CefJsBridgeRender instance.
class CefJSHandler : public CefV8Handler {
public:
    CefJSHandler() {}

    /// Executes a JavaScript function call from the renderer process.
    /// @param name The name of the function being called
    /// @param object The JavaScript object on which the function was invoked
    /// @param arguments List of arguments passed to the function
    /// @param retval Output parameter for the return value
    /// @param exception Output parameter for any exception message
    /// @return true if the function was handled successfully, false otherwise
    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) override;

    /// Registers the JavaScript bridge that will handle the actual function execution.
    /// @param jsBridge Shared pointer to the CefJsBridgeRender instance
    void registerJsBridge(std::shared_ptr<CefJsBridgeRender> jsBridge) {
        _jsBridge = jsBridge;
    }

    IMPLEMENT_REFCOUNTING(CefJSHandler);

protected:
    std::shared_ptr<CefJsBridgeRender> _jsBridge;  ///< Bridge instance for handling JS calls
};

}  // namespace cefview
