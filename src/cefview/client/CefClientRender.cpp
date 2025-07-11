#include "CefClientRender.h"

#include <map>
#include <array>
#include <string>
#include <iostream>
#include <fstream>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"

#include "CefSwitches.h"
#include "CefJsBridgeRender.h"
#include "CefJsHandler.h"

namespace cef
{

#define UNCAUGHT_EXCEPTION_LOG_FILE_NAME "browser_uncaught_exception_"
CefString AppGetTempDirectory();

CefString AppGetUncaughtExceptionLogFileName() {
    CefString tmpPath = AppGetTempDirectory();
    auto nowt = time(NULL);
    tm nowtm;
    ::localtime_s(&nowtm, &nowt);
    std::array<char, 16> buffer;
    ::strftime(buffer.data(), buffer.size(), "%Y-%m-%d", &nowtm);
    return tmpPath.ToString().append(UNCAUGHT_EXCEPTION_LOG_FILE_NAME).append(buffer.data()).append(".log");
}

std::shared_ptr<std::ofstream> getLogFileStream() {
  static std::shared_ptr<std::ofstream> logFileStream(
        new std::ofstream(AppGetUncaughtExceptionLogFileName().ToString(), std::ios::app),
        [](std::ofstream *ptr) { ptr->close(); delete ptr; });
  return logFileStream;
}
//////////////////////////////////////////////////////////////////////////////////////////
// CefRenderProcessHandler methods.
class ClientRenderDelegate : public ClientApp::RenderDelegate {
public:
    ClientRenderDelegate()
      : lastNodeIsEditable_(false) {
    }

    virtual void onWebKitInitialized(CefRefPtr<ClientApp> app) override
    {
        // DWORD pid = GetCurrentProcessId();
        // DWORD tid = GetCurrentThreadId();
        // std::cout << "ClientApp::OnWebKitInitialized (Process id " << pid << "), (Thread id " << tid << ")" << std::endl;

        // Register the client_app extension.
        CefRefPtr<ClientAppExtensionHandler> appHandler(new ClientAppExtensionHandler(this));
        std::string appCode =
            "var app;"
            "if (!app)"
            "  app = {};"
            "(function() {"
            "  app.sendMessage = function(name, arguments) {"
            "    native function sendMessage();"
            "    return sendMessage(name, arguments);"
            "  };"
            "  app.setMessageCallback = function(name, callback) {"
            "    native function setMessageCallback();"
            "    return setMessageCallback(name, callback);"
            "  };"
            "  app.removeMessageCallback = function(name) {"
            "    native function removeMessageCallback();"
            "    return removeMessageCallback(name);"
            "  };"
            "})();";
        CefRegisterExtension("v8/app", appCode, appHandler.get());


        /**
        * JavaScript 扩展代码，这里定义一个 NimCefWebFunction 对象提供 call 方法来让 Web 端触发 native 的 CefV8Handler 处理代码
        * param[in] functionName	要调用的 C++ 方法名称
        * param[in] params			调用该方法传递的参数，在前端指定的是一个 Object，但转到 Native 的时候转为了字符串
        * param[in] callback		执行该方法后的回调函数
        * 前端调用示例
        * NimCefWebHelper.call('showMessage', { message: 'Hello C++' }, (arguments) => {
        *    console.log(arguments)
        * })
        */
        std::string extensionCode = R"(
            var CefWebExtension = {};
            (() => {
                CefWebExtension.call = (functionName, arg1, arg2) => {
                    if (typeof arg1 === 'function') {
                        native function call(functionName, arg1);
                        return call(functionName, arg1);
                    } else {
                        const jsonString = JSON.stringify(arg1);
                        native function call(functionName, jsonString, arg2);
                        return call(functionName, jsonString, arg2);
                    }
                };
                CefWebExtension.register = (functionName, callback) => {
                    native function register(functionName, callback);
                    return register(functionName, callback);
                };
            })();
        )";
        CefRefPtr<CefJSHandler> handler = new CefJSHandler();

        if (!_renderJsBridge.get())
            _renderJsBridge.reset(new CefJsBridgeRender);
        handler->AttachJSBridge(_renderJsBridge);
        CefRegisterExtension("v8/extern", extensionCode, handler);
    }

    virtual void onBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info)
    {
        if (!_renderJsBridge.get())
            _renderJsBridge.reset(new CefJsBridgeRender);
    }

    virtual void onContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
    {
        _renderJsBridge->removeCallbackFuncWithFrame(frame);
        _renderJsBridge->unRegisterJSFuncWithFrame(frame);

        // Remove any JavaScript callbacks registered for the context that has been
        // released.
        if (!_callbackMap.empty()) {
            CallbackMap::iterator it = _callbackMap.begin();
            for (; it != _callbackMap.end();) {
                if (it->second.first->IsSame(context))
                    _callbackMap.erase(it++);
                else
                    ++it;
            }
        }
    }

    virtual void onFocusedNodeChanged(CefRefPtr<ClientApp> app, CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) override
    {
        bool isEditable = (node.get() && node->IsEditable());
        if (isEditable != lastNodeIsEditable_) {
            // Notify the browser of the change in focused element type.
            lastNodeIsEditable_ = isEditable;
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kFocusedNodeChangedMessage);
            message->GetArgumentList()->SetBool(0, isEditable);
            frame->SendProcessMessage(PID_BROWSER, message);
        }
    }

    virtual void onUncaughtException(
        CefRefPtr<ClientApp> app,
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context,
        CefRefPtr<CefV8Exception> exception,
        CefRefPtr<CefV8StackTrace> stackTrace) override
    {

        std::shared_ptr<std::ofstream> logFileStream = getLogFileStream();
        std::ofstream &out = *logFileStream;
        for (int i = 0; i != stackTrace->GetFrameCount(); ++i) {
        CefRefPtr<CefV8StackFrame> frame = stackTrace->GetFrame(i);
            out << frame->GetFunctionName().ToString() << " "
                << frame->GetLineNumber() << ":" << frame->GetColumn() << std::endl;
        }
    }

    virtual bool onProcessMessageReceived(
        CefRefPtr<ClientApp> app,
        CefRefPtr<CefBrowser> browser,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override
    {
        assert(source_process == PID_BROWSER);
        // 收到 browser 的消息回复
        const CefString& messageName = message->GetName();
        if (messageName == kExecuteJsCallbackMessage)
        {
            int         callbackId	= message->GetArgumentList()->GetInt(0);
            bool        hasError	= message->GetArgumentList()->GetBool(1);
            CefString	jsonString = message->GetArgumentList()->GetString(2);

            // 将收到的参数通过管理器传递给调用时传递的回调函数
            _renderJsBridge->executeJSCallbackFunc(callbackId, hasError, jsonString);
        }
        else if (messageName == kCallJsFunctionMessage)
        {
            CefString functionName = message->GetArgumentList()->GetString(0);
            CefString jsonString = message->GetArgumentList()->GetString(1);
            int cppCallbackId = message->GetArgumentList()->GetInt(2);
            CefString frameId = message->GetArgumentList()->GetString(3);

            // 通过 C++ 执行一个已经注册过的 JS 方法
            // frame_id 小于 0 则可能是 browser 进程的 browser 是无效的，所以这里为了避免出现错误就获取一个顶层 frame 执行代码
            _renderJsBridge->executeJSFunc(functionName, jsonString, frameId.empty() ? browser->GetMainFrame() : browser->GetFrameByIdentifier(frameId), cppCallbackId);
        }

        return false;
    }

    virtual void setMessageCallback(
        const std::string& messageName,
        int browserId,
        CefRefPtr<CefV8Context> context,
        CefRefPtr<CefV8Value> function) override
    {
        assert(CefCurrentlyOn(TID_RENDERER));

        _callbackMap.insert(std::make_pair(std::make_pair(messageName, browserId), std::make_pair(context, function)));
    }

    virtual bool removeMessageCallback(const std::string& messageName, int browserId) override
    {
        assert(CefCurrentlyOn(TID_RENDERER));

        CallbackMap::iterator it = _callbackMap.find(std::make_pair(messageName, browserId));
        if (it != _callbackMap.end()) {
            _callbackMap.erase(it);
            return true;
        }

        return false;
    }

private:
    bool lastNodeIsEditable_;
    // Map of message callbacks.
    typedef std::map<std::pair<std::string, int>,
                     std::pair<CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value>>> CallbackMap;
    CallbackMap _callbackMap;

    std::shared_ptr<CefJsBridgeRender> _renderJsBridge;

    IMPLEMENT_REFCOUNTING(ClientRenderDelegate);
};

void CreateRenderDelegatesInner(ClientApp::RenderDelegateSet& delegates) {
  delegates.insert(new ClientRenderDelegate);
}

#if defined(OS_WIN)
CefString AppGetTempDirectory() {
  std::unique_ptr<TCHAR[]> buffer(new TCHAR[MAX_PATH + 1]);
  if (!::GetTempPath(MAX_PATH, buffer.get())) return CefString();
  else return CefString(buffer.get());
}
#elif defined(OS_MACOSX)
CefString AppGetTempDirectory() {
  return CefString();
}
#endif

}