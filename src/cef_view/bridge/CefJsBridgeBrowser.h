#pragma once
#include "include/cef_app.h"

#include <map>
#include <functional>

namespace cefview {

/// Callback function type for handling JavaScript function execution results
typedef std::function<void(const std::string& result)> CallJsFunctionCallback;

/// C++ function type that can be called from JavaScript, takes JSON params and returns JSON result
typedef std::function<std::string&(const std::string& jsonParams)> CppFunction;

/// Map of C++ callback IDs to their corresponding callback functions
typedef std::map<int, CallJsFunctionCallback> BrowserCallbackMap;typedef std::map<int/* cppCallbackId*/, CallJsFunctionCallback/* callback*/> BrowserCallbackMap;

/// Map of function name and browser ID pairs to registered C++ functions
typedef std::map<std::pair<CefString/* functionName*/, int/* browserId*/>, CppFunction/* function*/> BrowserRegisteredFunction;

/// CefJsBridgeBrowser manages the JavaScript-C++ bridge in the browser process.
/// It handles bidirectional communication between JavaScript and C++ code,
/// including function calls, callbacks, and function registration.
class CefJsBridgeBrowser {
public:
    CefJsBridgeBrowser();
    ~CefJsBridgeBrowser();

    /// Calls a JavaScript function that has been registered in the renderer process.
    /// @param jsFunctionName The name of the JavaScript function to call
    /// @param params JSON-formatted parameters to pass to the function
    /// @param frame The frame in which to execute the JavaScript code
    /// @param callback Callback function to receive the result from JavaScript
    /// @return true if the execution request was successfully initiated, false if the callback ID already exists
    bool callJSFunction(const CefString& jsFunctionName, const CefString& params,
                        CefRefPtr<CefFrame> frame, CallJsFunctionCallback callback);

    /// Executes a C++ callback function identified by its ID with the provided result data.
    /// @param cppCallbackId The unique identifier of the callback function
    /// @param jsonString JSON-formatted result data from JavaScript
    /// @return true if the callback was executed successfully, false if the callback doesn't exist
    bool executeCppCallbackFunc(int cppCallbackId, const CefString& jsonString);

    /// Registers a persistent C++ function that can be called from JavaScript.
    /// @param functionName The name of the function to expose to JavaScript
    /// @param function The C++ function implementation
    /// @param browser The browser instance associated with this function
    /// @param replace Whether to replace an existing function with the same name (default: false)
    /// @return true if registration succeeded, false if the function name already exists (when replace=false)
    bool registerCppFunc(const CefString& functionName, CppFunction function,
                         CefRefPtr<CefBrowser> browser, bool replace = false);

    /// Unregisters a previously registered C++ function.
    /// @param functionName The name of the function to unregister
    /// @param browser The browser instance associated with this function
    void unRegisterCppFunc(const CefString& functionName, CefRefPtr<CefBrowser> browser);

    /// Executes a registered C++ function when a JavaScript call request is received.
    /// @param functionName The name of the C++ function to execute
    /// @param params JSON-formatted parameters from JavaScript
    /// @param jsCallbackId The callback ID to return results to JavaScript
    /// @param browser The browser instance handle
    /// @return true if execution succeeded, false if the function doesn't exist
    bool executeCppFunc(const CefString& functionName, const CefString& params,
                        int jsCallbackId, CefRefPtr<CefBrowser> browser);

private:
    uint32_t _jsCallbackId{0};     ///< Counter for generating unique JavaScript callback IDs
    uint32_t _cppCallbackId{0};    ///< Counter for generating unique C++ callback IDs

    BrowserCallbackMap _browserCallback;                ///< Map of pending C++ callbacks
    BrowserRegisteredFunction _browserRegisteredFunction;   ///< Map of registered C++ functions
};

}  // namespace cefview