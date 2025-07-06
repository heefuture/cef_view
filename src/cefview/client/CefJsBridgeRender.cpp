#include "stdafx.h"
#include "CefJsBridgeRender.h"
//#include "ipc_string_define.h"

namespace cef {

CefJsBridgeRender::CefJsBridgeRender()
{
}

CefJsBridgeRender::~CefJsBridgeRender()
{
}

bool CefJsBridgeRender::CallCppFunction(const CefString& function_name, const CefString& params, CefRefPtr<CefV8Value> callback)
{
    auto it = render_callback_.find(js_callback_id_);
    if (it == render_callback_.cend())
    {
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kCallCppFunctionMessage);

        message->GetArgumentList()->SetString(0, function_name);
        message->GetArgumentList()->SetString(1, params);
        message->GetArgumentList()->SetInt(2, js_callback_id_);

        render_callback_.emplace(js_callback_id_++, std::make_pair(context, callback));

        // 发送消息到 browser 进程
        CefRefPtr<CefBrowser> browser = context->GetBrowser();
        browser->SendProcessMessage(PID_BROWSER, message);

        return true;
    }

    return false;
}

void CefJsBridgeRender::RemoveCallbackFuncWithFrame(CefRefPtr<CefFrame> frame)
{
    if (!render_callback_.empty())
    {
        for (auto it = render_callback_.begin(); it != render_callback_.end();)
        {
            if (it->second.first->IsSame(frame->GetV8Context()))
            {
                it = render_callback_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

bool CefJsBridgeRender::ExecuteJSCallbackFunc(int js_callback_id, bool has_error, const CefString& json_result)
{
    auto it = render_callback_.find(js_callback_id);
    if (it != render_callback_.cend())
    {
        auto context = it->second.first;
        auto callback = it->second.second;

        if (context.get() && callback.get())
        {
            context->Enter();

            CefV8ValueList arguments;

            // 第一个参数标记函数执行结果是否成功
            arguments.push_back(CefV8Value::CreateBool(has_error));

            // 第二个参数携带函数执行后返回的数据
            CefV8ValueList json_parse_args;
            json_parse_args.push_back(CefV8Value::CreateString(json_result));
            CefRefPtr<CefV8Value> json_parse = context->GetGlobal()->GetValue("JSON")->GetValue("parse");
            CefRefPtr<CefV8Value> json_object = json_parse->ExecuteFunction(NULL, json_parse_args);
            arguments.push_back(json_object);

            // 执行 JS 方法
            CefRefPtr<CefV8Value> retval = callback->ExecuteFunction(NULL, arguments);
            if (retval.get())
            {
                if (retval->IsBool())
                {
                    retval->GetBoolValue();
                }
            }

            context->Exit();

            // 从列表中移除 callback 缓存
            render_callback_.erase(js_callback_id);

            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool CefJsBridgeRender::RegisterJSFunc(const CefString& function_name, CefRefPtr<CefV8Value> function, bool replace/* = false*/)
{
    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    CefRefPtr<CefFrame> frame = context->GetFrame();

    if (replace) {
        render_registered_function_.emplace(std::make_pair(function_name, frame->GetIdentifier()), function);
        return true;
    }
    else {
        auto it = render_registered_function_.find(std::make_pair(function_name, frame->GetIdentifier()));
        if (it == render_registered_function_.cend())
        {
            render_registered_function_.emplace(std::make_pair(function_name, frame->GetIdentifier()), function);
            return true;
        }

        return false;
    }

    return false;
}

void CefJsBridgeRender::UnRegisterJSFunc(const CefString& function_name, CefRefPtr<CefFrame> frame)
{
    render_registered_function_.erase(std::make_pair(function_name, frame->GetIdentifier()));
}

void CefJsBridgeRender::UnRegisterJSFuncWithFrame(CefRefPtr<CefFrame> frame)
{
    // 所以这里获取的 browser 都是全局唯一的，可以根据这个 browser 获取所有 frame 和 context
    auto browser = frame->GetBrowser();
    if (!render_registered_function_.empty()) {
        for (auto it = render_registered_function_.begin(); it != render_registered_function_.end();) {
            auto child_frame = browser->GetFrame(it->first.second);
            if (child_frame.get() && child_frame->GetV8Context()->IsSame(frame->GetV8Context())) {
                it = render_registered_function_.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
}

bool CefJsBridgeRender::ExecuteJSFunc(const CefString& function_name, const CefString& json_params, CefRefPtr<CefFrame> frame, int cpp_callback_id)
{
    auto it = render_registered_function_.find(std::make_pair(function_name, frame->GetIdentifier()));
    if (it != render_registered_function_.cend()) {
        auto context = frame->GetV8Context();
        auto function = it->second;

        if (context.get() && function.get()) {
            context->Enter();

            CefV8ValueList arguments;
            // 将 C++ 传递过来的 JSON 转换成 Object
            CefV8ValueList json_parse_args;
            json_parse_args.push_back(CefV8Value::CreateString(json_params));
            CefRefPtr<CefV8Value> json_object = context->GetGlobal()->GetValue("JSON");
            CefRefPtr<CefV8Value> json_parse = json_object->GetValue("parse");
            CefRefPtr<CefV8Value> json_stringify = json_object->GetValue("stringify");
            CefRefPtr<CefV8Value> json_object_args = json_parse->ExecuteFunction(NULL, json_parse_args);
            arguments.push_back(json_object_args);

            // 执行回调函数
            CefRefPtr<CefV8Value> retval = function->ExecuteFunction(NULL, arguments);
            if (retval.get() && retval->IsObject()) {
                // 回复调用 JS 后的返回值
                CefV8ValueList json_stringify_args;
                json_stringify_args.push_back(retval);
                CefRefPtr<CefV8Value> json_string = json_stringify->ExecuteFunction(NULL, json_stringify_args);
                CefString str = json_string->GetStringValue();

                CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kExecuteCppCallbackMessage);
                CefRefPtr<CefListValue> args = message->GetArgumentList();
                args->SetString(0, json_string->GetStringValue());
                args->SetInt(1, cpp_callback_id);
                context->GetBrowser()->SendProcessMessage(PID_BROWSER, message);
            }

            context->Exit();
            return true;
        }

        return false;
    }

    return false;
}
}
