#include "CefJsBridgeRender.h"
#include "CefSwitches.h"

namespace cefview {

CefJsBridgeRender::CefJsBridgeRender()
{
}

CefJsBridgeRender::~CefJsBridgeRender()
{
}

bool CefJsBridgeRender::callCppFunction(const CefString& functionName, const CefString& params, CefRefPtr<CefV8Value> callback)
{
    auto it = _renderCallback.find(_jsCallbackId);
    if (it == _renderCallback.cend())
    {
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kCallCppFunctionMessage);

        message->GetArgumentList()->SetString(0, functionName);
        message->GetArgumentList()->SetString(1, params);
        message->GetArgumentList()->SetInt(2, _jsCallbackId);

        _renderCallback.emplace(_jsCallbackId++, std::make_pair(context, callback));

        // 发送消息到 browser 进程
        CefRefPtr<CefBrowser> browser = context->GetBrowser();
        browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);

        return true;
    }

    return false;
}

void CefJsBridgeRender::removeCallbackFuncWithFrame(CefRefPtr<CefFrame> frame)
{
    if (!_renderCallback.empty())
    {
        for (auto it = _renderCallback.begin(); it != _renderCallback.end();)
        {
            if (it->second.first->IsSame(frame->GetV8Context())) {
                it = _renderCallback.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

bool CefJsBridgeRender::executeJSCallbackFunc(int jsCallbackId, bool hasError, const CefString& jsonString)
{
    auto it = _renderCallback.find(jsCallbackId);
    if (it != _renderCallback.cend())
    {
        auto context = it->second.first;
        auto callback = it->second.second;

        if (context.get() && callback.get())
        {
            context->Enter();

            CefV8ValueList arguments;

            // 第一个参数标记函数执行结果是否成功
            arguments.push_back(CefV8Value::CreateBool(hasError));

            // 第二个参数携带函数执行后返回的数据
            CefV8ValueList jsonParseArgs;
            jsonParseArgs.push_back(CefV8Value::CreateString(jsonString));
            CefRefPtr<CefV8Value> jsonParse = context->GetGlobal()->GetValue("JSON")->GetValue("parse");
            CefRefPtr<CefV8Value> jsonObject = jsonParse->ExecuteFunction(nullptr, jsonParseArgs);
            arguments.push_back(jsonObject);

            // 执行 JS 方法
            CefRefPtr<CefV8Value> retval = callback->ExecuteFunction(nullptr, arguments);
            if (retval.get())
            {
                if (retval->IsBool())
                {
                    retval->GetBoolValue();
                }
            }

            context->Exit();

            // 从列表中移除 callback 缓存
            _renderCallback.erase(jsCallbackId);

            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool CefJsBridgeRender::registerJSFunc(const CefString& functionName, CefRefPtr<CefV8Value> function, bool replace/* = false*/)
{
    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    CefRefPtr<CefFrame> frame = context->GetFrame();

    if (replace) {
        _renderRegisteredFunction.emplace(std::make_pair(functionName, frame->GetIdentifier()), function);
        return true;
    }
    else {
        auto it = _renderRegisteredFunction.find(std::make_pair(functionName, frame->GetIdentifier()));
        if (it == _renderRegisteredFunction.cend())
        {
            _renderRegisteredFunction.emplace(std::make_pair(functionName, frame->GetIdentifier()), function);
            return true;
        }

        return false;
    }

    return false;
}

void CefJsBridgeRender::unRegisterJSFunc(const CefString& functionName, CefRefPtr<CefFrame> frame)
{
    _renderRegisteredFunction.erase(std::make_pair(functionName, frame->GetIdentifier()));
}

void CefJsBridgeRender::unRegisterJSFuncWithFrame(CefRefPtr<CefFrame> frame)
{
    // 所以这里获取的 browser 都是全局唯一的，可以根据这个 browser 获取所有 frame 和 context
    auto browser = frame->GetBrowser();
    if (!_renderRegisteredFunction.empty()) {
        for (auto it = _renderRegisteredFunction.begin(); it != _renderRegisteredFunction.end();) {
            auto childFrame = browser->GetFrameByIdentifier(it->first.second);
            if (childFrame.get() && childFrame->GetV8Context()->IsSame(frame->GetV8Context())) {
                it = _renderRegisteredFunction.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
}

bool CefJsBridgeRender::executeJSFunc(const CefString& functionName, const CefString& jsonParams, CefRefPtr<CefFrame> frame, int cppCallbackid)
{
    auto it = _renderRegisteredFunction.find(std::make_pair(functionName, frame->GetIdentifier()));
    if (it != _renderRegisteredFunction.cend()) {
        auto context = frame->GetV8Context();
        auto function = it->second;

        if (context.get() && function.get()) {
            context->Enter();

            CefV8ValueList arguments;
            // 将 C++ 传递过来的 JSON 转换成 Object
            CefV8ValueList jsonParseArgs;
            jsonParseArgs.push_back(CefV8Value::CreateString(jsonParams));
            CefRefPtr<CefV8Value> jsonObject = context->GetGlobal()->GetValue("JSON");
            CefRefPtr<CefV8Value> jsonParse = jsonObject->GetValue("parse");
            CefRefPtr<CefV8Value> jsonStringify = jsonObject->GetValue("stringify");
            CefRefPtr<CefV8Value> jsonObjectArgs = jsonParse->ExecuteFunction(nullptr, jsonParseArgs);
            arguments.push_back(jsonObjectArgs);

            // 执行回调函数
            CefRefPtr<CefV8Value> retval = function->ExecuteFunction(nullptr, arguments);
            if (retval.get() && retval->IsObject()) {
                // 回复调用 JS 后的返回值
                CefV8ValueList jsonStringifyArgs;
                jsonStringifyArgs.push_back(retval);
                CefRefPtr<CefV8Value> jsonString = jsonStringify->ExecuteFunction(nullptr, jsonStringifyArgs);
                CefString str = jsonString->GetStringValue();

                CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kExecuteCppCallbackMessage);
                CefRefPtr<CefListValue> args = message->GetArgumentList();
                args->SetString(0, jsonString->GetStringValue());
                args->SetInt(1, cppCallbackid);
                context->GetBrowser()->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);
            }

            context->Exit();
            return true;
        }

        return false;
    }

    return false;
}
}
