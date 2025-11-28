#pragma once
#include "include/cef_app.h"

#include <map>

namespace cefview
{

typedef std::map<int/* jsCallbackid*/, std::pair<CefRefPtr<CefV8Context>/* context*/, CefRefPtr<CefV8Value>/* callback*/>> RenderCallbackMap;
typedef std::map<std::pair<CefString/* functionName*/, CefString/* frameId*/>, CefRefPtr<CefV8Value>/* function*/> RenderRegisteredFunction;


// in render process
class CefJsBridgeRender
{
public:
   CefJsBridgeRender();
   ~CefJsBridgeRender();

   /**
    * Execute a registered C++ method
    * @param[in] functionName  Function name to call
    * @param[in] params        JSON format parameters
    * @param[in] callback      Result callback function after execution
    * @return true if request initiated successfully (doesn't guarantee execution success, check callback), false if callback ID already exists
    */
   bool callCppFunction(const CefString& functionName, const CefString& params, CefRefPtr<CefV8Value> callback);

   /**
    * Remove specified callback functions by context (triggered on page refresh)
    * @param[in] frame  Current running frame
    */
   void removeCallbackFuncWithFrame(CefRefPtr<CefFrame> frame);

   /**
    * Execute callback function by ID
    * @param[in] jsCallbackId  Callback function ID
    * @param[in] hasError      Whether there's an error (first parameter of callback)
    * @param[in] jsonString    JSON string data if no error (second parameter of callback)
    * @return true if callback executed successfully, false if callback doesn't exist or execution context is invalid
    */
   bool executeJSCallbackFunc(int jsCallbackId, bool hasError, const CefString& jsonString);

   /**
    * Register a persistent JS function for C++ to call
    * @param[in] functionName  Function name, provided as string for C++ to call directly, must be unique
    * @param[in] context       Function execution context
    * @param[in] function      Function body
    * @param[in] replace       Whether to replace if same name exists, default false
    * @return When replace is true, returns true for successful replacement, false for unpredictable behavior. When replace is false, returns true for successful registration, false if name already registered
    */
   bool registerJSFunc(const CefString& functionName, CefRefPtr<CefV8Value> function, bool replace = false);

   /**
    * Unregister a persistent JS function
    * @param[in] functionName  Function name
    * @param[in] frame         Frame to unregister function from
    */
   void unRegisterJSFunc(const CefString& functionName, CefRefPtr<CefFrame> frame);

   /**
   * Unregister one or more persistent JS functions by execution context
   * @param[in] frame  Current running frame
   */
   void unRegisterJSFuncWithFrame(CefRefPtr<CefFrame> frame);

   /**
    * Execute a specific JS function by name
    * @param[in] functionName      Function name
    * @param[in] jsonParams        JSON format parameters to pass
    * @param[in] frame             Frame to execute JS function in
    * @param[in] cppCallbackId     C++ callback function ID to call after execution
    * @return true if JS function executed successfully, false if function doesn't exist or execution context is invalid
    */
   bool executeJSFunc(const CefString& functionName, const CefString& jsonParams, CefRefPtr<CefFrame> frame, int cppCallbackId);

private:
   uint32_t                      _jsCallbackId{0};            // JS callback function index counter
   uint32_t                      _cppCallbackId{0};           // C++ callback function index counter

   RenderCallbackMap             _renderCallback;               // JS callback function mapping list
   RenderRegisteredFunction      _renderRegisteredFunction;    // List of registered persistent JS functions
};

}