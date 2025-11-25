//#include "CefJsBridgeBrowser.h"
//#include <client/CefSwitches.h>
//
//namespace cefview
//{
//
//CefJsBridgeBrowser::CefJsBridgeBrowser()
//{
//
//}
//
//CefJsBridgeBrowser::~CefJsBridgeBrowser()
//{
//
//}
//
//bool CefJsBridgeBrowser::callJSFunction(const CefString& jsFunctionName, const CefString& params, CefRefPtr<CefFrame> frame, CallJsFunctionCallback callback)
//{
//    if (!frame.get()) {
//        return false;
//    }
//
//    auto it = _browserCallback.find(_cppCallbackId);
//    if (it == _browserCallback.cend())
//    {
//        _browserCallback.emplace(_cppCallbackId, callback);
//        // 发送消息给 render 要求执行一个 js function
//        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kCallJsFunctionMessage);
//        CefRefPtr<CefListValue> args = message->GetArgumentList();
//        args->SetString(0, jsFunctionName);
//        args->SetString(1, params);
//        args->SetInt(2, _cppCallbackId++);
//        args->SetString(3, frame->GetIdentifier());
//
//        frame->SendProcessMessage(PID_RENDERER, message);
//
//        return true;
//    }
//
//    return false;
//}
//
//bool CefJsBridgeBrowser::executeCppCallbackFunc(int cppCallbackId, const CefString& jsonString)
//{
//    auto it = _browserCallback.find(cppCallbackId);
//    if (it != _browserCallback.cend()) {
//        auto callback = it->second;
//        if (callback) {
//            callback(jsonString);
//        }
//
//        // 执行完成后从缓存中移除
//        _browserCallback.erase(cppCallbackId);
//    }
//
//    return false;
//}
//
//bool CefJsBridgeBrowser::registerCppFunc(const CefString& functionName, CppFunction function, CefRefPtr<CefBrowser> browser, bool replace /*= false*/)
//{
//    if (replace) {
//        _browserRegisteredFunction.emplace(std::make_pair(functionName, browser ? browser->GetIdentifier() : -1), function);// = ;
//        return true;
//    }
//    else {
//        auto it = _browserRegisteredFunction.find(std::make_pair(functionName, browser ? browser->GetIdentifier() : -1));
//        if (it == _browserRegisteredFunction.cend()) {
//            _browserRegisteredFunction.emplace(std::make_pair(functionName, browser ? browser->GetIdentifier() : -1), function);
//            return true;
//        }
//
//        return false;
//    }
//
//    return false;
//}
//
//void CefJsBridgeBrowser::unRegisterCppFunc(const CefString& functionName, CefRefPtr<CefBrowser> browser)
//{
//    _browserRegisteredFunction.erase(std::make_pair(functionName, browser ? browser->GetIdentifier() : -1));
//}
//
//bool CefJsBridgeBrowser::executeCppFunc(const CefString& functionName, const CefString& params, int jsCallbackId, CefRefPtr<CefBrowser> browser)
//{
//    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kExecuteJsCallbackMessage);
//    CefRefPtr<CefListValue> args = message->GetArgumentList();
//
//    auto it = _browserRegisteredFunction.find(std::make_pair(functionName, browser->GetIdentifier()));
//    if (it != _browserRegisteredFunction.cend()) {
//        auto function = it->second;
//        std::string& result = function(params);
//        args->SetInt(0, jsCallbackId);
//        args->SetBool(1, true);
//        args->SetString(2, result);
//        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
//
//        return true;
//    }
//
//    it = _browserRegisteredFunction.find(std::make_pair(functionName, -1));
//    if (it != _browserRegisteredFunction.cend()) {
//        auto function = it->second;
//         std::string& result = function(params);
//        args->SetInt(0, jsCallbackId);
//        args->SetBool(1, true);
//        args->SetString(2, result);
//        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
//        return true;
//    }
//    else {
//        args->SetInt(0, jsCallbackId);
//        args->SetBool(1, true);
//        args->SetString(2, R"({"message":"Function does not exist."})");
//        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
//        return false;
//    }
//}
//
//}
