/**
* @file        ProcessMessageDelegate.h
* @brief       ProcessMessageDelegate 类的定义
*              用于处理进程消息的委托类
* @version     1.0
* @author      heefuture
* @date        2025.07.02
* @copyright
*/
#ifndef PROCESSMESSAGEDELEGATE_H
#define PROCESSMESSAGEDELEGATE_H
#pragma once

#include <include/cef_client.h>
#include <memory>

namespace cefview {
class ProcessMessageDelegate : public std::enable_shared_from_this<ProcessMessageDelegate>,
{
public:
    ProcessMessageDelegate(cefview::ProcessMessageHandler* msgHandler);
    bool onProcessMessageReceived(
        CefRefPtr<CefHandler> handler,
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

private:
    std::shared_ptr<cefview::ProcessMessageHandler> _handler;
};
}
#endif //!PROCESSMESSAGEDELEGATE_H