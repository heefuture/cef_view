#pragma once
#include "include/cef_app.h"

#include <map>
#include <functional>
namespace cef
{

typedef std::function<void(const std::string& result)> CallJsFunctionCallback;
typedef std::function<std::string&(const std::string& jsonParams)> CppFunction;

typedef std::map<int/* cppCallbackId*/, CallJsFunctionCallback/* callback*/> BrowserCallbackMap;
typedef std::map<std::pair<CefString/* functionName*/, int/* browserId*/>, CppFunction/* function*/> BrowserRegisteredFunction;

// in browser process
class CefJsBridgeBrowser
{
public:
    CefJsBridgeBrowser();
    ~CefJsBridgeBrowser();

    /**
     * 执行已经注册好的 JS 方法
     * param[in] jsFunctionName 要调用的 JS 函数名称
     * param[in] params         调用 JS 方法传递的 json 格式参数
     * param[in] frame          调用哪个框架下的 JS 代码
     * param[in] callback       调用 JS 方法后返回数据的回调函数
     * return 返回 ture 标识发起执行 JS 函数命令成功，返回 false 是相同的 callback id 已经存在
     */
    bool callJSFunction(const CefString& jsFunctionName, const CefString& params, CefRefPtr<CefFrame> frame, CallJsFunctionCallback callback);

    /**
     * 根据 ID 执行指定回调函数
     * param[in] cppCallbackId callback 函数的 id
     * param[in] jsonString	返回的 json 格式数据
     * return 返回 true 执行成功，false 为执行失败，可能回调不存在
     */
    bool executeCppCallbackFunc(int cppCallbackId, const CefString& jsonString);

    /**
     * 注册一个持久的 C++ 函数提供 JS 端调用
     * param[in] functionName  要提供 JS 调用的函数名字
     * param[in] function       函数体
     * param[in] replace        是否替换相同名称的函数体，默认不替换
     * return replace 为 true 的情况下，返回 true 表示注册或者替换成功，false 是不可预知行为。replace 为 false 的情况下返回 true 表示注册成功，返回 false 表示函数名已经注册
     */
    bool registerCppFunc(const CefString& functionName, CppFunction function, CefRefPtr<CefBrowser> browser, bool  replace = false);

    /**
     * 反注册一个持久的 C++ 函数
     * param[in] function_name  要反注册的函数名称
     */
    void unRegisterCppFunc(const CefString& functionName, CefRefPtr<CefBrowser> browser);

    /**
     * 执行一个已经注册好的 C++ 方法（接受到 JS 端执行请求时被调用）
     * param[in] functionName  要执行的函数名称
     * param[in] param          携带的参数
     * param[in] jsCallbackId 回调 JS 端所需的回调函数 ID
     * param[in] browser        browser 实例句柄
     * return 返回 true 表示执行成功，返回 false 表示执行失败，函数名可能不存在
     */
    bool executeCppFunc(const CefString& functionName, const CefString& params, int js_jsCallbackIdcallback_id, CefRefPtr<CefBrowser> browser);

private:
    uint32_t                    _jsCallbackId{0};             // JS 端回调函数的索引计数
    uint32_t                    _cppCallbackId{0};            // C++ 端回调函数的索引计数


    BrowserCallbackMap          _browserCallback;              // C++ 端回调函数的对应列表
    BrowserRegisteredFunction   _browserRegisteredFunction;   // 保存 C++ 端已经注册好的持久函数列表
};

}