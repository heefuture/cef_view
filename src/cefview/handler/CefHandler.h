/**
* @file        CefHandler.h
* @brief       CefClient的实现类，处理Cef浏览器对象发出的各个事件和消息，并与上层控件进行数据交互
* @version     1.0
* @author      heefuture
* @date        2025.06.26
* @copyright
*/
#ifndef CEFHANDLER_H
#define CEFHANDLER_H
#pragma once
#include <set>
#include <list>
#include "include/cef_client.h"
#include "include/cef_browser.h"

namespace cef {
// CefHandler implements CefClient and a number of other interfaces.
class CefHandler : public CefClient,
                      public CefLifeSpanHandler,
                      public CefRenderHandler,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDragHandler,
                      public CefKeyboardHandler,
                      public CefLoadHandler,
                      public CefRequestHandler,
                      public CefDownloadHandler
{
public:
    CefHandler();

    // Interface for process message delegates. Do not perform work in the
    // RenderDelegate constructor.
    class ProcessMessageDelegate : public CefBaseRefCounted {
    public:
        // Called when a process message is received. Return true if the message was
        // handled and should not be passed on to other handlers.
        // ProcessMessageDelegates should check for unique message names to avoid
        // interfering with each other.
        virtual bool onProcessMessageReceived(
            CefRefPtr<CefHandler> handler,
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefProcessId source_process,
            CefRefPtr<CefProcessMessage> message) {
            return false;
        }

        // upgrade attention, modified by N11016;
        IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
    };
    typedef std::set<CefRefPtr<ProcessMessageDelegate>> ProcessMessageDelegateSet;

    /** @class HandlerDelegate
     * @brief ClientHandler的消息委托类接口，ClientHandler把这些事件传递到委托接口中
     *        可以根据需求来扩展此接口
     */
    class HandlerDelegate
    {
    public:
        // 在非UI线程中被调用
        virtual void onPaint(CefRefPtr<CefBrowser> browser,
            CefRenderHandler::PaintElementType type,
            const CefRenderHandler::RectList& dirtyRects,
            const void* buffer,
            int width,
            int height) = 0;

        virtual void onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
            PaintElementType type,
            const RectList& dirtyRects,
            const CefAcceleratedPaintInfo &info) = 0;

        virtual bool getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) = 0;

        virtual void getViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) = 0;

        virtual bool getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) = 0;

        virtual bool getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) = 0;

        virtual void onPopupShow(CefRefPtr<CefBrowser> browser, bool show) = 0;

        virtual void onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) = 0;

        // 在非UI线程中被调用
        virtual void onBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) = 0;

        // 在非UI线程中被调用
        virtual bool onContextMenuCommand(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefContextMenuParams> params,
            int command_id,
            EventFlags event_flags) = 0;

        virtual void onAddressChange(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, const CefString& url) = 0;

        virtual void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) = 0;

        virtual bool onCursorChange(CefRefPtr<CefBrowser> browser,
                              CefCursorHandle cursor,
                              cef_cursor_type_t type,
                              const CefCursorInfo& custom_cursor_info) = 0;

        virtual void onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) = 0;

        virtual void onLoadStart(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, TransitionType transition_type) = 0;

        virtual void onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) = 0;

        virtual void onLoadError(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            ErrorCode errorCode,
            const CefString& errorText,
            const CefString& failedUrl) = 0;

        // 在非UI线程中被调用
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
            bool *no_javascript_access) = 0;

        virtual bool onAfterCreated(CefRefPtr<CefBrowser> browser) = 0;

        virtual void onBeforeClose(CefRefPtr<CefBrowser> browser) = 0;

        // 在非UI线程中被调用
        virtual bool onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) = 0;

        virtual void onRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status, int error_code, const CefString& error_string) = 0;

        // 文件下载相关
        virtual bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefDownloadItem> download_item,
            const CefString& suggested_name,
            CefRefPtr<CefBeforeDownloadCallback> callback) = 0;

        virtual void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefDownloadItem> download_item,
            CefRefPtr<CefDownloadItemCallback> callback) = 0;

        // 封装一些 JS 与 C++ 交互的功能
        //virtual bool onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser) = 0;

        //virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string) = 0;
    };

public:
    // 设置消息处理委托对象
    virtual void RegisterProcessMessageDelegates(CefRefPtr<ProcessMessageDelegate> process_handler) {
        process_message_delegates_.insert(process_handler);
    };

    // 设置委托类指针，浏览器对象的一些事件会交给此指针对象来处理
    // 当指针所指的对象不需要处理事件时，应该给参数传入NULL
    void SetHandlerDelegate(HandlerDelegate *handler){ handle_delegate_ = handler; }

    // 设置Cef渲染内容的大小
    void NotifyRectUpdated();

    // 当前Web页面中获取焦点的元素是否可编辑
    bool IsCurFieldEditable(){ return is_focus_oneditable_field_; }

    CefRefPtr<CefBrowser> GetBrowser(){ return browser_; }

    CefRefPtr<CefBrowserHost> GetBrowserHost();

    void CloseAllBrowser();

    bool IsClosing() const { return is_closing_; }
public:

    // CefClient methods. Important to return |this| for the handler callbacks.
    virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }
    virtual CefRefPtr<CefDragHandler> GetDragHandler() override { return this; }
    virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
    virtual CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

    // CefLifeSpanHandler methods
    virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
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

    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

    virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;

    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // CefRenderHandler methods
    virtual bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

    virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

    virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) override;

    virtual bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) override;

    virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;

    virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override;

    virtual void OnPaint(CefRefPtr<CefBrowser> browser,	PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) override;

    virtual void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser,
        PaintElementType type,
        const RectList &dirtyRects,
        const CefAcceleratedPaintInfo &info) override;

    virtual bool OnCursorChange(CefRefPtr<CefBrowser> browser,
        CefCursorHandle cursor,
        cef_cursor_type_t type,
        const CefCursorInfo &custom_cursor_info) override;

    // virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
    //     CefRefPtr<CefDragData> drag_data,
    //     CefRenderHandler::DragOperationsMask allowed_ops,
    //     int x,
    //     int y) override;
    //
    // virtual void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
    //     CefRenderHandler::DragOperation operation) override;

    // CefContextMenuHandler methods
    virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params,
        CefRefPtr<CefMenuModel> model) override;

    virtual bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params,
        int command_id,
        EventFlags event_flags) override;

    // CefDisplayHandler methods
    virtual void OnAddressChange(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const CefString& url) override;

    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;

    virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
        cef_log_severity_t level,
        const CefString& message,
        const CefString& source,
        int line) override;

    // CefLoadHandler methods
    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
        bool isLoading,
        bool canGoBack,
        bool canGoForward) override;

    virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        TransitionType transition_type) override;

    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;

    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        ErrorCode errorCode,
        const CefString& errorText,
        const CefString& failedUrl) override;

    // CefRequestHandler methods
    virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool user_gesture,
        bool is_redirect) override;

    virtual void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
        TerminationStatus status,
        int error_code,
        const CefString& error_string) override;

    // CefDownloadHandler methods
    virtual bool OnBeforeDownload(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        const CefString& suggested_name,
        CefRefPtr<CefBeforeDownloadCallback> callback) override;

    virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        CefRefPtr<CefDownloadItemCallback> callback) override;

    static int s_browser_count;
protected:
    int                                 browser_id_{0};
    CefRefPtr<CefBrowser>               browser_;
    // List of any popup browser windows. Only accessed on the CEF UI thread.
    typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
    BrowserList                         popup_browsers_;
    // Registered delegates.
    ProcessMessageDelegateSet           process_message_delegates_;
    HandlerDelegate*                    handle_delegate_{nullptr};
    bool                                is_focus_oneditable_field_{false};
    bool                                is_closing_{false};
    //client::DropTargetHandle drop_target_;
    //CefRenderHandler::DragOperation current_drag_op_;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(CefHandler);
};
}

#endif //!CEFHANDLER_H
