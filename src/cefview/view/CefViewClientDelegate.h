/**
* @file        CefViewClientDelegate.h
* @brief       CefViewClientDelegate 类的定义
*              用于处理 CefHandler 浏览器的基本操作和事件
* @version     1.0
* @author      heefuture
* @date        2025.07.08
* @copyright   Copyright (C) 2025 Tencent. All rights reserved.
*/
#ifndef CEFVIEWCLIENTDELEGATE_H
#define CEFVIEWCLIENTDELEGATE_H
#pragma once
#include <client/CefViewClient.h>
#include <client/CefViewClientDelegateBase.h>
// #include <view/ProcessMessageDelegateWrapper.h>

#include "include/cef_base.h"

#include <memory>
#include <functional>

namespace cefview {
class CefWebView;

class CefViewClientDelegate : public CefViewClientDelegateBase
{
public:
    CefViewClientDelegate();
    virtual ~CefViewClientDelegate();

public:
    CefRefPtr<CefBrowser> getCefBrowser() const;
    /**
     * @brief 加载一个地址
     * @param[in] url 网站地址
     */
    void loadUrl(const std::string& url);

    /**
     * @brief 获取页面 URL
     * @return 返回 URL 地址
     */
    const std::string& getUrl() const;

    /**
     * @brief 重设浏览器大小
     * @param[in] width 新的宽度
     * @param[in] height 新的高度
     */
    virtual void resize(int width, int height);

    // /**
    // * @brief 注册一个 ProcessMessageHandler 对象，主要用来处理js消息
    // * @param [in] handler ProcessMessageHandler 对象指针
    // */
    // virtual void registerProcessMessageHandler(ProcessMessageHandler* handler);

    /**
     * @brief 打开开发者工具
     * @return 成功返回 true，失败返回 false
     */
    virtual bool openDevTools();

    /**
     * @brief 关闭开发者工具
     * @return 无
     */
    virtual void closeDevTools();

    /**
     * @brief 判断是否打开开发者工具
     * @return 返回 true 表示已经绑定，false 为未绑定
     */
    virtual bool isDevToolsOpened() const { return _isDevtoolsOpened; }

    // virtual bool onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser) override;

    // virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string) override;

private:
#pragma region CefClient
    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) override;
#pragma endregion // CefClient

#pragma region CefContextMenuHandler
    virtual void onBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) override;

    // 在非UI线程中被调用
    virtual bool onContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefContextMenuParams> params,
                                      int command_id,
                                      CefContextMenuHandler::EventFlags event_flags) override;
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
    virtual void onAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url) override;

    virtual void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) override;

    virtual bool onCursorChange(CefRefPtr<CefBrowser> browser,
                                CefCursorHandle cursor,
                                cef_cursor_type_t type,
                                const CefCursorInfo &custom_cursor_info) override;

    virtual bool onConsoleMessage(CefRefPtr<CefBrowser> browser,
                                  cef_log_severity_t level,
                                  const CefString& message,
                                  const CefString& source,
                                  int line) override;
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
    virtual bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        const CefString &suggested_name,
        CefRefPtr<CefBeforeDownloadCallback> callback) override;

    virtual void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        CefRefPtr<CefDownloadItemCallback> callback) override;
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
    virtual bool onDragEnter(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> drag_data,
                             CefRenderHandler::DragOperationsMask mask) override;
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
    virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event) override;
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
    virtual bool onBeforePopup(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               int popup_id,
                               const CefString &target_url,
                               const CefString &target_frame_name,
                               CefLifeSpanHandler::WindowOpenDisposition target_disposition,
                               bool user_gesture,
                               const CefPopupFeatures &popupFeatures,
                               CefWindowInfo &windowInfo,
                               CefRefPtr<CefClient> &client,
                               CefBrowserSettings &settings,
                               CefRefPtr<CefDictionaryValue> &extra_info,
                               bool *no_javascript_access) override;

    virtual bool onAfterCreated(CefRefPtr<CefBrowser> browser) override;

    virtual void onBeforeClose(CefRefPtr<CefBrowser> browser) override;
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
    virtual void onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override;

    virtual void onLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transition_type) override;

    virtual void onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;

    virtual void onLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefLoadHandler::ErrorCode errorCode,
                             const CefString &errorText,
                             const CefString &failedUrl) override;
#pragma endregion // CefLoadHandler

#pragma region CefRenderHandler
    virtual void onPaint(CefRefPtr<CefBrowser> browser,
        ::CefRenderHandler::PaintElementType type,
        const ::CefRenderHandler::RectList &dirtyRects,
        const void *buffer,
        int width,
        int height) override;

    virtual void onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
        ::CefRenderHandler::PaintElementType type,
        const ::CefRenderHandler::RectList &dirtyRects,
        const CefAcceleratedPaintInfo &info) override;

    virtual bool getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;

    virtual void getViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;

    virtual bool getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) override;

    virtual bool getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) override;

    virtual void onPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;

    virtual void onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect) override;

    virtual bool startDragging(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDragData> drag_data,
        CefRenderHandler::DragOperationsMask allowed_ops,
        int x, int y) override;

    virtual void updateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) override;

    virtual void onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
        const CefRange& selection_range,
        const CefRenderHandler::RectList& character_bounds) override;
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
    virtual bool onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) override;

    virtual void onRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int error_code, const CefString& error_string) override;
#pragma endregion // CefRequestHandler

    //// 封装一些 JS 与 C++ 交互的功能
    //virtual bool onExecuteCppFunc(const CefString &function_name, const CefString &params, int js_callback_id, CefRefPtr<CefBrowser> browser) override;

    //virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString &json_string) override;
    //enum WebState { kNone, kCreating, kCreated };
    //WebState _webstate;
protected:
    HWND                                    _hwnd; // Native window handle for the CefWebView
    std::weak_ptr<CefWebView>               _view;
    typedef std::function<void(void)>       StdClosure;
    std::vector<StdClosure>                 _taskListAfterCreated;
    // std::vector<CefRefPtr<ProcessMessageDelegateWrapper>> _delegates;
    CefRefPtr<CefViewClient>                _cefViewCLient;
    //std::shared_ptr<CefJSBridge> js_bridge_;
    CefString                               _url;
    bool                                    _isDevtoolsOpened{false};
    //int                         js_callback_thread_id_ = -1; // 保存接收到 JS 调用 CPP 函数的代码所属线程，以后触发 JS 回调时把回调转到那个线程
};
}

#endif //!CEFVIEWCLIENTDELEGATE_H
