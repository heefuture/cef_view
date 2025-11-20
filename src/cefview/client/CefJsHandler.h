/** @file js_handler.h
 * @brief 定义一个C++与JS进行交互的JsHandler类
 * @copyright (c) 2016, NetEase Inc. All rights reserved
 * @author Redrain
 * @date 2016/7/19
 */
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

    void AttachJSBridge(std::shared_ptr<CefJsBridgeRender> jsBridge) { _jsBridge = jsBridge; }

    IMPLEMENT_REFCOUNTING(CefJSHandler);
private:
    std::shared_ptr<CefJsBridgeRender> _jsBridge;
};

// Handles the native implementation for the client_app extension.
class ClientAppExtensionHandler : public CefV8Handler
{
public:
    explicit ClientAppExtensionHandler(const std::shared_ptr<CefViewAppDelegateInterface>& renderDelegate)
        : _renderDelegate(renderDelegate)
    {
    }

    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception) override;

private:
    std::weak_ptr<CefViewAppDelegateInterface> _renderDelegate;

    IMPLEMENT_REFCOUNTING(ClientAppExtensionHandler);
};
}

