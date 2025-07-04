#pragma once
#include "include/cef_app.h"

#include <map>

namespace cef
{

typedef std::map<int/* js_callback_id*/, std::pair<CefRefPtr<CefV8Context>/* context*/, CefRefPtr<CefV8Value>/* callback*/>> RenderCallbackMap;
typedef std::map<std::pair<CefString/* function_name*/, int64_t/* frame_id*/>, CefRefPtr<CefV8Value>/* function*/> RenderRegisteredFunction;


// in render process
class CefJsBridgeRender : public CefBaseRefCounted
{
public:
    CefJsBridgeRender();
    ~CefJsBridgeRender();

    /**
     * 执行已经注册好的 C++ 方法
     * param[in] function_name  要调用的函数名称
     * param[in] params         调用该函数传递的 json 格式参数
     * param[in] callback       执行完成后的结果回调函数
     * return 返回 true 表示发起执行请求成功（并不代表一定执行成功，具体看回调），返回 false 可能是注册的回调函数 ID 已经存在
     */
    bool CallCppFunction(const CefString& function_name, const CefString& params, CefRefPtr<CefV8Value> callback);

    /**
     * 通过判断上下文环境移除指定回调函数（页面刷新会触发该方法）
     * param[in] frame          当前运行框架
     */
    void RemoveCallbackFuncWithFrame(CefRefPtr<CefFrame> frame);

    /**
     * 根据 ID 执行指定回调函数
     * param[in] js_callback_id 回调函数的 ID
     * param[in] has_error      是否有错误，对应回调函数第一个参数
     * param[in] json_string    如果没有错误则返回指定的 json string 格式的数据，对应回调函数第二个参数
     * return 返回 true 表示成功执行了回调函数，返回 false 有可能回调函数不存在或者回调函数所需的执行上下文环境已经不存在
     */
    bool ExecuteJSCallbackFunc(int js_callback_id, bool has_error, const CefString& json_result);

    /**
     * 注册一个持久的 JS 函数提供 C++ 调用
     * param[in] function_name  函数名称，字符串形式提供 C++ 直接调用，名称不能重复
     * param[in] context        函数的执行上下文环境
     * param[in] function       函数体
     * param[in] replace        若已经存在该名称的函数是否替换，默认否
     * return replace 为 true 的情况下，返回 true 是替换成功，返回 false 为不可预见行为。replace 为 false 的情况下返回 true 表示注册成功，返回 false 是同名函数已经注册过了。
     */
    bool RegisterJSFunc(const CefString& function_name, CefRefPtr<CefV8Value> function, bool replace = false);

    /**
     * 反注册一个持久的 JS 函数
     * param[in] function_name  函数名称
     * param[in] frame          要取消注册哪个框架下的相关函数
     */
    void UnRegisterJSFunc(const CefString& function_name, CefRefPtr<CefFrame> frame);

    /**
    * 根据执行上下文反注册一个或多个持久的 JS 函数
    * param[in] frame           当前运行所属框架
    */
    void UnRegisterJSFuncWithFrame(CefRefPtr<CefFrame> frame);

    /**
     * 根据名称执行某个具体的 JS 函数
     * param[in] function_name      函数名称
     * param[in] json_params        要传递的 json 格式的参数
     * param[in] frame              执行哪个框架下的 JS 函数
     * param[in] cpp_callback_id    执行完成后要回调的 C++ 回调函数 ID
     * return 返回 true 表示成功执行某个 JS 函数，返回 false 有可能要执行的函数不存在或者该函数的运行上下文已经无效
     */
    bool ExecuteJSFunc(const CefString& function_name, const CefString& json_params, CefRefPtr<CefFrame> frame, int cpp_callback_id);

private:
    uint32_t                      js_callback_id_{0};            // JS 端回调函数的索引计数
    uint32_t                      cpp_callback_id_{0};           // C++ 端回调函数的索引计数

    RenderCallbackMap             render_callback_;               // JS 端回调函数的对应列表
    RenderRegisteredFunction      render_registered_function_;    // 保存 JS 端已经注册好的持久函数列表
};

}