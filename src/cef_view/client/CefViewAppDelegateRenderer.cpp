#include "CefViewAppDelegateRenderer.h"

#include <string>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"

#include <client/CefSwitches.h>
#include <bridge/CefJsBridgeRender.h>
#include <bridge/CefJsHandler.h>

namespace cefview {

void CefViewAppDelegateRenderer::onWebKitInitialized() {
    // DWORD pid = GetCurrentProcessId();
    // DWORD tid = GetCurrentThreadId();
    // std::cout << "ClientApp::OnWebKitInitialized (Process id " << pid << "), (Thread id " << tid << ")" << std::endl;

    // Register the client_app extension.
    CefRefPtr<CefJSHandler> appHandler(new CefJSHandler());
    std::string appCode =
        "var cefViewApp;"
        "if (!cefViewApp)"
        "  cefViewApp = {};"
        "(function() {"
        "  cefViewApp.sendMessage = function(name, arguments) {"
        "    native function sendMessage();"
        "    return sendMessage(name, arguments);"
        "  };"
        "  cefViewApp.setMessageCallback = function(name, callback) {"
        "    native function setMessageCallback();"
        "    return setMessageCallback(name, callback);"
        "  };"
        "  cefViewApp.removeMessageCallback = function(name) {"
        "    native function removeMessageCallback();"
        "    return removeMessageCallback(name);"
        "  };"
        "  cefViewApp.call = (functionName, arg1, arg2) => {"
        "    if (typeof arg1 === 'function') {"
        "      native function call(functionName, arg1);"
        "      return call(functionName, arg1);"
        "    } else {"
        "      const jsonString = JSON.stringify(arg1);"
        "      native function call(functionName, jsonString, arg2);"
        "      return call(functionName, jsonString, arg2);"
        "    }"
        "  };"
        "  cefViewApp.register = (functionName, callback) => {"
        "    native function register(functionName, callback);"
        "    return register(functionName, callback);"
        "  };"
        "  cefViewApp.unRegister = (functionName) => {"
        "    native function unRegister(functionName);"
        "    return unRegister(functionName);"
        "  };"
        "})();";

    if (!_renderJsBridge)
        _renderJsBridge.reset(new cefview::CefJsBridgeRender);

    appHandler->registerJsBridge(_renderJsBridge);
    CefRegisterExtension("v8/cefViewApp", appCode, appHandler.get());
}

void CefViewAppDelegateRenderer::onBrowserCreated(CefRefPtr<CefBrowser> browser,
                                                  CefRefPtr<CefDictionaryValue> extraInfo) {
    if (!_renderJsBridge) {
        _renderJsBridge = std::make_shared<CefJsBridgeRender>();
    }
}

void CefViewAppDelegateRenderer::onContextReleased(CefRefPtr<CefBrowser> browser,
                                                   CefRefPtr<CefFrame> frame,
                                                   CefRefPtr<CefV8Context> context) {
    _renderJsBridge->removeCallbackFuncWithFrame(frame);
    _renderJsBridge->unRegisterJSFuncWithFrame(frame);
}

void CefViewAppDelegateRenderer::onFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                                      CefRefPtr<CefFrame> frame,
                                                      CefRefPtr<CefDOMNode> node) {
    bool isEditable = (node.get() && node->IsEditable());
    if (isEditable != _lastNodeIsEditable) {
        // Notify the browser of the change in focused element type.
        _lastNodeIsEditable = isEditable;
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kFocusedNodeChangedMessage);
        message->GetArgumentList()->SetBool(0, isEditable);
        frame->SendProcessMessage(PID_BROWSER, message);
    }
}

void CefViewAppDelegateRenderer::onUncaughtException(CefRefPtr<CefBrowser> browser,
                                                     CefRefPtr<CefFrame> frame,
                                                     CefRefPtr<CefV8Context> context,
                                                     CefRefPtr<CefV8Exception> exception,
                                                     CefRefPtr<CefV8StackTrace> stackTrace) {
    //    std::shared_ptr<std::ofstream> logFileStream = getLogFileStream();
    //    std::ofstream &out = *logFileStream;
    //    for (int i = 0; i != stackTrace->GetFrameCount(); ++i) {
    //    CefRefPtr<CefV8StackFrame> frame = stackTrace->GetFrame(i);
    //        out << frame->GetFunctionName().ToString() << " "
    //            << frame->GetLineNumber() << ":" << frame->GetColumn() << std::endl;
    //    }
}

bool CefViewAppDelegateRenderer::onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                                          CefProcessId sourceProcess,
                                                          CefRefPtr<CefProcessMessage> message) {
    assert(sourceProcess == PID_BROWSER);
    // Received message reply from browser process
    const CefString& messageName = message->GetName();
    if (messageName == kExecuteJsCallbackMessage) {
        int callbackId = message->GetArgumentList()->GetInt(0);
        bool hasError = message->GetArgumentList()->GetBool(1);
        CefString jsonString = message->GetArgumentList()->GetString(2);

        // Pass received parameters to callback function via manager
        _renderJsBridge->executeJSCallbackFunc(callbackId, hasError, jsonString);
    } else if (messageName == kCallJsFunctionMessage) {
        CefString functionName = message->GetArgumentList()->GetString(0);
        CefString jsonString = message->GetArgumentList()->GetString(1);
        int cppCallbackId = message->GetArgumentList()->GetInt(2);
        CefString frameId = message->GetArgumentList()->GetString(3);

        // Execute a registered JS function from C++
        // If frame_id is invalid (browser process browser may be invalid), get main frame to execute
        _renderJsBridge->executeJSFunc(functionName, jsonString,
                                       frameId.empty() ? browser->GetMainFrame() : browser->GetFrameByIdentifier(frameId),
                                       cppCallbackId);
    }

    return false;
}

}  // namespace cefview