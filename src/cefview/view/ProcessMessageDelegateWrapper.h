/**
* @file        ProcessMessageDelegateWrapper.h
* @brief       ProcessMessageDelegateWrapper 类的定义
*              用于处理进程消息的委托包装类
* @version     1.0
* @author      heefuture
* @date        2025.07.02
* @copyright
*/
#ifndef PROCESSMESSAGEDELEGATEWRAPPER_H
#define PROCESSMESSAGEDELEGATEWRAPPER_H
#pragma once

#include <handler/CefHandler.h>
#include <view/ProcessMessageHandler.h>
#include <memory>

namespace cef {
class ProcessMessageDelegateWrapper : public CefHandler::ProcessMessageDelegate {
public:
    ProcessMessageDelegateWrapper(cef::ProcessMessageHandler* msgHandler);
    bool onProcessMessageReceived(
        CefRefPtr<CefHandler> handler,
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

    IMPLEMENT_REFCOUNTING(ProcessMessageDelegateWrapper);
private:
    std::shared_ptr<cef::ProcessMessageHandler> _handler;
};
}
#endif //!PROCESSMESSAGEDELEGATEWRAPPER_H