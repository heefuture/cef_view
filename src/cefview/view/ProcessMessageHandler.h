
/**
* @file        ProcessMessageHandler.h
* @brief       ProcessMessageHandler 类的定义
*              用于处理进程消息的委托接口
* @version     1.0
* @author      heefuture
* @date        2025.07.03
* @copyright
*/
#ifndef PROCESSMESSAGEHANDLER_H
#define PROCESSMESSAGEHANDLER_H
#pragma once

#include <memory>
#include <string>

namespace cefview {

    class ProcessMessageHandler {
    public:
        virtual void releaseAndDelete() = 0;
        virtual std::string handleProcessMessage(const std::string& request) = 0;
    };
}

#endif //!PROCESSMESSAGEHANDLER_H

