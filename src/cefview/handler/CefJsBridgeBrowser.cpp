#include "stdafx.h"
#include "CefJsBridgeBrowser.h"
//#include "ipc_string_define.h"

namespace cef
{

CefJsBridgeBrowser::CefJsBridgeBrowser()
{

}

CefJsBridgeBrowser::~CefJsBridgeBrowser()
{

}

bool CefJsBridgeBrowser::CallJSFunction(const CefString& js_function_name, const CefString& params, CefRefPtr<CefFrame> frame, CallJsFunctionCallback callback)
{
    if (!frame.get()) {
        return false;
    }

    auto it = browser_callback_.find(cpp_callback_id_);
    if (it == browser_callback_.cend())
    {
        browser_callback_.emplace(cpp_callback_id_, callback);
        // 发送消息给 render 要求执行一个 js function
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kCallJsFunctionMessage);
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        args->SetString(0, js_function_name);
        args->SetString(1, params);
        args->SetInt(2, cpp_callback_id_++);
        args->SetString(3, frame->GetIdentifier());

        frame->SendProcessMessage(PID_RENDERER, message);

        return true;
    }

    return false;
}

bool CefJsBridgeBrowser::ExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string)
{
    auto it = browser_callback_.find(cpp_callback_id);
    if (it != browser_callback_.cend()) {
        auto callback = it->second;
        if (callback) {
            callback(json_string);
        }

        // 执行完成后从缓存中移除
        browser_callback_.erase(cpp_callback_id);
    }

    return false;
}

bool CefJsBridgeBrowser::RegisterCppFunc(const CefString& function_name, CppFunction function, CefRefPtr<CefBrowser> browser, bool replace /*= false*/)
{
    if (replace) {
        browser_registered_function_.emplace(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1), function);// = ;
        return true;
    }
    else {
        auto it = browser_registered_function_.find(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1));
        if (it == browser_registered_function_.cend()) {
            browser_registered_function_.emplace(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1), function);
            return true;
        }

        return false;
    }

    return false;
}

void CefJsBridgeBrowser::UnRegisterCppFunc(const CefString& function_name, CefRefPtr<CefBrowser> browser)
{
    browser_registered_function_.erase(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1));
}

bool CefJsBridgeBrowser::ExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser)
{
    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kExecuteJsCallbackMessage);
    CefRefPtr<CefListValue> args = message->GetArgumentList();

    auto it = browser_registered_function_.find(std::make_pair(function_name, browser->GetIdentifier()));
    if (it != browser_registered_function_.cend()) {
        auto function = it->second;
        std::string& result = function(params);
        args->SetInt(0, js_callback_id);
        args->SetBool(1, true);
        args->SetString(2, result);
        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);

        return true;
    }

    it = browser_registered_function_.find(std::make_pair(function_name, -1));
    if (it != browser_registered_function_.cend()) {
        auto function = it->second;
         std::string& result = function(params);
        args->SetInt(0, js_callback_id);
        args->SetBool(1, true);
        args->SetString(2, result);
        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
        return true;
    }
    else {
        args->SetInt(0, js_callback_id);
        args->SetBool(1, true);
        args->SetString(2, R"({"message":"Function does not exist."})");
        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
        return false;
    }
}

}
