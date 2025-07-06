#pragma once
#include "include/cef_app.h"

#include <map>
namespace cef
{

typedef std::function<void(const std::string& result)> CallJsFunctionCallback;
typedef std::function<std::string&(const std::string& params)> CppFunction;

typedef std::map<int/* cpp_callback_id*/, CallJsFunctionCallback/* callback*/> BrowserCallbackMap;
typedef std::map<std::pair<CefString/* function_name*/, int64_t/* browser_id*/>, CppFunction/* function*/> BrowserRegisteredFunction;

// in browser process
class CefJsBridgeBrowser : public CefBaseRefCounted
{
public:
    CefJsBridgeBrowser();
    ~CefJsBridgeBrowser();

    /**
     * 执行已经注册好的 JS 方法
     * param[in] js_function_name 要调用的 JS 函数名称
     * param[in] params         调用 JS 方法传递的 json 格式参数
     * param[in] frame          调用哪个框架下的 JS 代码
     * param[in] callback       调用 JS 方法后返回数据的回调函数
     * return 返回 ture 标识发起执行 JS 函数命令成功，返回 false 是相同的 callback id 已经存在
     */
    bool CallJSFunction(const CefString& js_function_name, const CefString& params, CefRefPtr<CefFrame> frame, CallJsFunctionCallback callback);

    /**
     * 根据 ID 执行指定回调函数
     * param[in] cpp_callback_id callback 函数的 id
     * param[in] json_string	返回的 json 格式数据
     * return 返回 true 执行成功，false 为执行失败，可能回调不存在
     */
    bool ExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string);

    /**
     * 注册一个持久的 C++ 函数提供 JS 端调用
     * param[in] function_name  要提供 JS 调用的函数名字
     * param[in] function       函数体
     * param[in] replace        是否替换相同名称的函数体，默认不替换
     * return replace 为 true 的情况下，返回 true 表示注册或者替换成功，false 是不可预知行为。replace 为 false 的情况下返回 true 表示注册成功，返回 false 表示函数名已经注册
     */
    bool RegisterCppFunc(const CefString& function_name, CppFunction function, CefRefPtr<CefBrowser> browser, bool  replace = false);

    /**
     * 反注册一个持久的 C++ 函数
     * param[in] function_name  要反注册的函数名称
     */
    void UnRegisterCppFunc(const CefString& function_name, CefRefPtr<CefBrowser> browser);

    /**
     * 执行一个已经注册好的 C++ 方法（接受到 JS 端执行请求时被调用）
     * param[in] function_name  要执行的函数名称
     * param[in] param          携带的参数
     * param[in] js_callback_id 回调 JS 端所需的回调函数 ID
     * param[in] browser        browser 实例句柄
     * return 返回 true 表示执行成功，返回 false 表示执行失败，函数名可能不存在
     */
    bool ExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser);

private:
    uint32_t                    js_callback_id_{0};             // JS 端回调函数的索引计数
    uint32_t                    cpp_callback_id_{0};            // C++ 端回调函数的索引计数


    BrowserCallbackMap          browser_callback_;              // C++ 端回调函数的对应列表
    BrowserRegisteredFunction   browser_registered_function_;   // 保存 C++ 端已经注册好的持久函数列表
};

}