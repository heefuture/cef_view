/**
* @file        CefViewClientDelegateBase.h
* @brief       CefViewClientDelegateBase class declaration
* @version     1.0
* @author      heefuture
* @date        2025.09.04
* @copyright
*/
#ifndef CEFVIEWCLIENTDELEGATEBASE_H
#define CEFVIEWCLIENTDELEGATEBASE_H
#pragma once
#include <memory>
#include <include/cef_client.h>

namespace cefview {

// Interface for browser delegates. All BrowserDelegates must be returned via
// CreateBrowserDelegates. Do not perform work in the BrowserDelegate
// constructor. See CefBrowserProcessHandler for documentation.
class CefViewClientDelegateBase {
public:
    typedef std::shared_ptr<CefViewClientDelegateBase> RefPtr;
    typedef std::weak_ptr<CefViewClientDelegateBase> WeakPtr;
    virtual ~CefViewClientDelegateBase() {}

#pragma region CefClient
    /////////////////////////////////////// CefClient methods: /////////////////////////////////////
    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) { return false; }
#pragma endregion // CefClient

#pragma region CefRenderHandler
    /////////////////////////////////////// CefRenderHandler methods: /////////////////////////////////////
    virtual void onPaint(CefRefPtr<CefBrowser> browser,
                         CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height) = 0;

    virtual void onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
                                    CefRenderHandler::PaintElementType type,
                                    const CefRenderHandler::RectList& dirtyRects,
                                    const CefAcceleratedPaintInfo &info) = 0;

    virtual bool getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) = 0;

    virtual void getViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) = 0;

    virtual bool getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) = 0;

    virtual bool getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) = 0;

    virtual void onPopupShow(CefRefPtr<CefBrowser> browser, bool show) = 0;

    virtual void onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) = 0;

    virtual bool startDragging(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefDragData> drag_data,
                               CefRenderHandler::DragOperationsMask allowed_ops,
                               int x, int y) = 0;

    virtual void updateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) = 0;

    virtual void onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
                                              const CefRange& selection_range,
                                              const CefRenderHandler::RectList& character_bounds) = 0;
#pragma endregion // CefRenderHandler

#pragma region CefContextMenuHandler
    /////////////////////////////////////// CefContextMenuHandler methods: /////////////////////////////////////
    // 在非UI线程中被调用
    virtual void onBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) = 0;

    // 在非UI线程中被调用
    virtual bool onContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefContextMenuParams> params,
                                      int command_id,
                                      CefContextMenuHandler::EventFlags event_flags) = 0;
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
    /////////////////////////////////////// CefDisplayHandler methods: /////////////////////////////////////
        virtual void onAddressChange(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, const CefString& url) = 0;

        virtual void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) = 0;

        virtual bool onCursorChange(CefRefPtr<CefBrowser> browser,
                                    CefCursorHandle cursor,
                                    cef_cursor_type_t type,
                                    const CefCursorInfo& custom_cursor_info) = 0;

        virtual bool onConsoleMessage(CefRefPtr<CefBrowser> browser,
                                    cef_log_severity_t level,
                                    const CefString& message,
                                    const CefString& source,
                                    int line) = 0;
#pragma endregion // CefDisplayHandler

#pragma region CefLoadHandler
    /////////////////////////////////////// CefLoadHandler methods: /////////////////////////////////////
    virtual void onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) = 0;

    virtual void onLoadStart(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transition_type) = 0;

    virtual void onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) = 0;

    virtual void onLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefLoadHandler::ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl) = 0;
#pragma endregion // CefLoadHandler

#pragma region CefLifeSpanHandler
    /////////////////////////////////////// CefLifeSpanHandler methods: /////////////////////////////////////
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
#pragma endregion // CefLifeSpanHandler

#pragma region CefRequestHandler
    /////////////////////////////////////// CefRequestHandler methods: /////////////////////////////////////
    // 在非UI线程中被调用
    virtual bool onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect) = 0;

    virtual void onRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                           CefRequestHandler::TerminationStatus status,
                                           int error_code,
                                           const CefString& error_string) = 0;
#pragma endregion // CefRequestHandler

#pragma region CefDownloadHandler
    /////////////////////////////////////// CefDownloadHandler methods: /////////////////////////////////////
    virtual bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefDownloadItem> download_item,
                                  const CefString& suggested_name,
                                  CefRefPtr<CefBeforeDownloadCallback> callback) = 0;

    virtual void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefDownloadItem> download_item,
                                   CefRefPtr<CefDownloadItemCallback> callback) = 0;
#pragma endregion // CefDownloadHandler

    // 封装一些 JS 与 C++ 交互的功能
    //virtual bool onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser) = 0;
    //virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string) = 0;
};

}  // namespace cefview

#endif //!CEFVIEWCLIENTDELEGATEBASE_H