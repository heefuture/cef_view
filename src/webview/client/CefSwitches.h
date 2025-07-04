#ifndef CEFSWITCHES_H
#define CEFSWITCHES_H
#pragma once

namespace cef {

/* Defines all of the command line switches used by cefclient. */
extern const char kNoProxyServer[];
extern const char kWinHttpProxyResolver[];
extern const char kMultiThreadedMessageLoop[];
extern const char kCachePath[];
extern const char kUrl[];
extern const char kExternalDevTools[];
extern const char kOffScreenRenderingEnabled[];
extern const char kTransparentPaintingEnabled[];
extern const char kMouseCursorChangeDisabled[];
extern const char kUncaughtExceptionStackSize[];
extern const char kLogSeverity[];
extern const char kLogFile[];

namespace log_severity {

extern const char kVerbose[];
extern const char kInfo[];
extern const char kWarning[];
extern const char kErrorReport[];
extern const char kDisable[];

} // namespace log_severity

extern const char kFocusedNodeChangedMessage[];  // web页面中获取焦点的元素改变
extern const char kExecuteCppCallbackMessage[];  // 执行 C++ 的消息回调函数
extern const char kCallCppFunctionMessage[];     // web调用C++接口接口的通知
extern const char kExecuteJsCallbackMessage[];   // web调用C++接口接口的通知
extern const char kCallJsFunctionMessage[];      // C++ 调用 JavaScript 通知

}  // namespace cef

#endif //!CEFSWITCHES_H
