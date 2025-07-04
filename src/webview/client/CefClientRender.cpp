#include "CefClientRender.h"

#include <string>
#include <iostream>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "cef_control/util/util.h"

//#include "cef_control/app/js_handler.h"
#include "CefSwitches.h"
#include "CefJsBridgeRender.h"

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

    virtual void onWebKitInitialized() override
    {

        CefRefPtr<CefJSHandler> handler = new CefJSHandler();

        if (!render_js_bridge_.get())
            render_js_bridge_.reset(new CefJSBridge);
        handler->AttachJSBridge(render_js_bridge_);
        CefRegisterExtension("v8/extern", extensionCode, handler);
    }

    virtual void onBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) override
    {
        if (!render_js_bridge_.get())
            render_js_bridge_.reset(new CefJSBridge);
    }

    virtual void onContextReleased(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override
    {
        render_js_bridge_->RemoveCallbackFuncWithFrame(frame);
        render_js_bridge_->UnRegisterJSFuncWithFrame(frame);


        // Remove any JavaScript callbacks registered for the context that has been
        // released.
        if (!callback_map_.empty()) {
            CallbackMap::iterator it = callback_map_.begin();
            for (; it != callback_map_.end();) {
                if (it->second.first->IsSame(context))
                    callback_map_.erase(it++);
                else
                    ++it;
            }
        }
    }

    virtual void onFocusedNodeChanged(CefRefPtr<ClientApp> app,
                                    CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefDOMNode> node) override {
        bool isEditable = (node.get() && node->IsEditable());
        if (isEditable != lastNodeIsEditable_) {
            // Notify the browser of the change in focused element type.
            lastNodeIsEditable_ = isEditable;
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kFocusedNodeChangedMessage);
            message->GetArgumentList()->SetBool(0, isEditable);
            frame->SendProcessMessage(PID_BROWSER, message);
        }
    }

    virtual void OnUncaughtException(CefRefPtr<ClientApp> app,
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context,
        CefRefPtr<CefV8Exception> exception,
        CefRefPtr<CefV8StackTrace> stackTrace) override {

        std::shared_ptr<std::ofstream> logFileStream = getLogFileStream();
        std::ofstream &out = *logFileStream;
        for (int i = 0; i != stackTrace->GetFrameCount(); ++i) {
        CefRefPtr<CefV8StackFrame> frame = stackTrace->GetFrame(i);
            out << frame->GetFunctionName().ToString() << " "
                << frame->GetLineNumber() << ":" << frame->GetColumn() << std::endl;
        }
    }

    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override
    {
        ASSERT(source_process == PID_BROWSER);
        // 收到 browser 的消息回复
        const CefString& message_name = message->GetName();
        if (message_name == kExecuteJsCallbackMessage)
        {
            int     callback_id	= message->GetArgumentList()->GetInt(0);
            bool        has_error	= message->GetArgumentList()->GetBool(1);
            CefString	json_string = message->GetArgumentList()->GetString(2);

            // 将收到的参数通过管理器传递给调用时传递的回调函数
            render_js_bridge_->ExecuteJSCallbackFunc(callback_id, has_error, json_string);
        }
        else if (message_name == kCallJsFunctionMessage)
        {
            CefString function_name = message->GetArgumentList()->GetString(0);
            CefString json_string = message->GetArgumentList()->GetString(1);
            int cpp_callback_id = message->GetArgumentList()->GetInt(2);
            int64 frame_id = message->GetArgumentList()->GetInt(3);

            // 通过 C++ 执行一个已经注册过的 JS 方法
            // frame_id 小于 0 则可能是 browser 进程的 browser 是无效的，所以这里为了避免出现错误就获取一个顶层 frame 执行代码
            render_js_bridge_->ExecuteJSFunc(function_name, json_string, frame_id < 0 ? browser->GetMainFrame() : browser->GetFrame(frame_id), cpp_callback_id);
        }

        return false;

        // 收到 browser 的消息回复
    const CefString& message_name = message->GetName();
    if (message_name == kExecuteJsCallbackMessage)
    {
        int         callback_id	= message->GetArgumentList()->GetInt(0);
        bool        has_error	= message->GetArgumentList()->GetBool(1);
        CefString   json_string = message->GetArgumentList()->GetString(2);

        // 将收到的参数通过管理器传递给调用时传递的回调函数
        render_js_bridge_->ExecuteJSCallbackFunc(callback_id, has_error, json_string);
    }
    else if (message_name == kCallJsFunctionMessage)
    {
        CefString function_name = message->GetArgumentList()->GetString(0);
        CefString json_string = message->GetArgumentList()->GetString(1);
        int cpp_callback_id = message->GetArgumentList()->GetInt(2);
        CefString frame_id = message->GetArgumentList()->GetInt(3);

        // 通过 C++ 执行一个已经注册过的 JS 方法
        // frame_id 小于 0 则可能是 browser 进程的 browser 是无效的，所以这里为了避免出现错误就获取一个顶层 frame 执行代码
        render_js_bridge_->ExecuteJSFunc(function_name, json_string, frame_id < 0 ? browser->GetMainFrame() : browser->GetFrame(frame_id), cpp_callback_id);
    }
    }

private:
    bool lastNodeIsEditable_;
    // Map of message callbacks.
    typedef std::map<std::pair<std::string, int>,
                     std::pair<CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value>>> CallbackMap;
    CallbackMap callback_map_;

    std::shared_ptr<CefJSBridge> render_js_bridge_;

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