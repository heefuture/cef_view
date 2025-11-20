#pragma once
#include "include/cef_app.h"

#include <map>

namespace cefview
{

typedef std::map<int/* jsCallbackid*/, std::pair<CefRefPtr<CefV8Context>/* context*/, CefRefPtr<CefV8Value>/* callback*/>> RenderCallbackMap;
typedef std::map<std::pair<CefString/* functionName*/, CefString/* frameId*/>, CefRefPtr<CefV8Value>/* function*/> RenderRegisteredFunction;


// in render process
class CefJsBridgeRender
{
public:
    CefJsBridgeRender();
    ~CefJsBridgeRender();

    /**
     * 执行已经注册好的 C++ 方法
     * param[in] functionName  要调用的函数名称
     * param[in] params         调用该函数传递的 json 格式参数
     * param[in] callback       执行完成后的结果回调函数
     * return 返回 true 表示发起执行请求成功（并不代表一定执行成功，具体看回调），返回 false 可能是注册的回调函数 ID 已经存在
     */
    bool callCppFunction(const CefString& functionName, const CefString& params, CefRefPtr<CefV8Value> callback);

    /**
     * 通过判断上下文环境移除指定回调函数（页面刷新会触发该方法）
     * param[in] frame          当前运行框架
     */
    void removeCallbackFuncWithFrame(CefRefPtr<CefFrame> frame);

    /**
     * 根据 ID 执行指定回调函数
     * param[in] jsCallbackId 回调函数的 ID
     * param[in] hasError      是否有错误，对应回调函数第一个参数
     * param[in] jsonString    如果没有错误则返回指定的 json string 格式的数据，对应回调函数第二个参数
     * return 返回 true 表示成功执行了回调函数，返回 false 有可能回调函数不存在或者回调函数所需的执行上下文环境已经不存在
     */
    bool executeJSCallbackFunc(int jsCallbackId, bool hasError, const CefString& jsonString);

    /**
     * 注册一个持久的 JS 函数提供 C++ 调用
     * param[in] functionName  函数名称，字符串形式提供 C++ 直接调用，名称不能重复
     * param[in] context        函数的执行上下文环境
     * param[in] function       函数体
     * param[in] replace        若已经存在该名称的函数是否替换，默认否
     * return replace 为 true 的情况下，返回 true 是替换成功，返回 false 为不可预见行为。replace 为 false 的情况下返回 true 表示注册成功，返回 false 是同名函数已经注册过了。
     */
    bool registerJSFunc(const CefString& functionName, CefRefPtr<CefV8Value> function, bool replace = false);

    /**
     * 反注册一个持久的 JS 函数
     * param[in] functionName  函数名称
     * param[in] frame          要取消注册哪个框架下的相关函数
     */
    void unRegisterJSFunc(const CefString& functionName, CefRefPtr<CefFrame> frame);

    /**
    * 根据执行上下文反注册一个或多个持久的 JS 函数
    * param[in] frame           当前运行所属框架
    */
    void unRegisterJSFuncWithFrame(CefRefPtr<CefFrame> frame);

    /**
     * 根据名称执行某个具体的 JS 函数
     * param[in] functionName      函数名称
     * param[in] jsonParams        要传递的 json 格式的参数
     * param[in] frame              执行哪个框架下的 JS 函数
     * param[in] cppCallbackId    执行完成后要回调的 C++ 回调函数 ID
     * return 返回 true 表示成功执行某个 JS 函数，返回 false 有可能要执行的函数不存在或者该函数的运行上下文已经无效
     */
    bool executeJSFunc(const CefString& functionName, const CefString& jsonParams, CefRefPtr<CefFrame> frame, int cppCallbackId);

private:
    uint32_t                      _jsCallbackId{0};            // JS 端回调函数的索引计数
    uint32_t                      _cppCallbackId{0};           // C++ 端回调函数的索引计数

    RenderCallbackMap             _renderCallback;               // JS 端回调函数的对应列表
    RenderRegisteredFunction      _renderRegisteredFunction;    // 保存 JS 端已经注册好的持久函数列表
};

}