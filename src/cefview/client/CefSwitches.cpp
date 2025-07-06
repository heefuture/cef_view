
#include "CefSwitches.h"

namespace cef {

// CEF and Chromium support a wide range of command-line switches. This file
// only contains command-line switches specific to the cefclient application.
// View CEF/Chromium documentation or search for *_switches.cc files in the
// Chromium source code to identify other existing command-line switches.
// Below is a partial listing of relevant *_switches.cc files:
//   base/base_switches.cc
//   cef/libcef/common/cef_switches.cc
//   chrome/common/chrome_switches.cc (not all apply)
//   content/public/common/content_switches.cc

const char kNoProxyServer[] = "no-proxy-server";
const char kWinHttpProxyResolver[] = "winhttp-proxy-resolver";
const char kMultiThreadedMessageLoop[] = "multi-threaded-message-loop";
const char kCachePath[] = "cache-path";
const char kUrl[] = "url";
const char kExternalDevTools[] = "external-devtools";
const char kOffScreenRenderingEnabled[] = "off-screen-rendering-enabled";
const char kTransparentPaintingEnabled[] = "transparent-painting-enabled";
const char kMouseCursorChangeDisabled[] = "mouse-cursor-change-disabled";
const char kUncaughtExceptionStackSize[] = "uncaught_exception_stack_size";
const char kLogSeverity[] = "log-severity";
const char kLogFile[] = "log-file";

namespace log_severity {

const char kVerbose[] = "verbose";
const char kInfo[] = "info";
const char kWarning[] = "warning";
const char kError[] = "error";
const char kErrorReport[] = "error-report";
const char kDisable[] = "disable";

} // namespace log_severity


const char kFocusedNodeChangedMessage[] = "FocusedNodeChanged";
const char kExecuteCppCallbackMessage[] = "ExecuteCppCallback";
const char kCallCppFunctionMessage[] = "CallCppFunction";
const char kExecuteJsCallbackMessage[] = "ExecuteJsCallback";
const char kCallJsFunctionMessage[] = "CallJsFunction";

}  // namespace cefclient
