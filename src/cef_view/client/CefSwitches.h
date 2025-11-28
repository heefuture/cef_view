/**
* @file        CefSwitches.h
* @brief       defines all of the command line switches used by cefView.
* @version     1.0
* @author      heefuture
* @date        2025.09.04
* @copyright
*/
#ifndef CEFSWITCHES_H
#define CEFSWITCHES_H
#pragma once

namespace cefview {

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

extern const char kFocusedNodeChangedMessage[];  // Focused element changed in web page
extern const char kExecuteCppCallbackMessage[];  // Execute C++ message callback function
extern const char kCallCppFunctionMessage[];     // Notification for web calling C++ interface
extern const char kExecuteJsCallbackMessage[];   // Notification for web calling C++ interface
extern const char kCallJsFunctionMessage[];      // Notification for C++ calling JavaScript

}  // namespace cefview

#endif //!CEFSWITCHES_H
