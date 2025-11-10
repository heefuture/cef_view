/**
* @file        CefViewClient.h
* @brief       CefClient的实现类，处理Cef浏览器对象发出的各个事件和消息，并与上层控件进行数据交互
* @version     1.0
* @author      heefuture
* @date        2025.06.26
* @copyright
*/
#ifndef CEFVIEWCLIENT_H
#define CEFVIEWCLIENT_H
#pragma once
#include <set>
#include <list>

#include <include/cef_client.h>
#include <include/cef_browser.h>

#include "CefViewClientDelegateBase.h"

namespace cefview {
// CefViewClient implements CefClient and a number of other interfaces.
class CefViewClient : public CefClient,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDownloadHandler,
                      public CefDragHandler,
                      public CefKeyboardHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRenderHandler,
                      public CefRequestHandler,

{
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(CefViewClient);
public:
    CefViewClient(CefViewClientDelegateBase::RefPtr delegate);
    ~CefViewClient();

    // 设置Cef渲染内容的大小
    void NotifyRectUpdated();

    // 当前Web页面中获取焦点的元素是否可编辑
    bool IsCurFieldEditable(){ return _isFocusOnEditableField; }

    CefRefPtr<CefBrowser> GetBrowser(){ return _browser; }

    CefRefPtr<CefBrowserHost> GetBrowserHost();

    void CloseAllBrowser();

    bool IsClosing() const { return _isClosing; }

public:
#pragma region CefClient
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
#pragma endregion // CefClient

#pragma region CefContextMenuHandler
    virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) override;

    virtual bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefContextMenuParams> params,
                                      int command_id,
                                      EventFlags event_flags) override;
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
    virtual void OnAddressChange(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 const CefString& url) override;

    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;

    virtual bool OnCursorChange(CefRefPtr<CefBrowser> browser,
                                CefCursorHandle cursor,
                                cef_cursor_type_t type,
                                const CefCursorInfo &custom_cursor_info) override;

    virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                                  cef_log_severity_t level,
                                  const CefString& message,
                                  const CefString& source,
                                 int line) override;
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
    virtual bool OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefDownloadItem> download_item,
                                  const CefString& suggested_name,
                                  CefRefPtr<CefBeforeDownloadCallback> callback) override;

    virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefDownloadItem> download_item,
                                   CefRefPtr<CefDownloadItemCallback> callback) override;
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
    virtual bool OnDragEnter(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> drag_data,
                             CefRenderHandler::DragOperationsMask mask) override;
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
    virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event) override;
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
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
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
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
#pragma endregion // CefLoadHandler

#pragma region CefRenderHandler
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

    virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefDragData> drag_data,
                               CefRenderHandler::DragOperationsMask allowed_ops,
                               int x, int y) override;

    virtual void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                                  CefRenderHandler::DragOperation operation) override;

    virtual void OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
                                              const CefRange& selection_range,
                                              const CefRenderHandler::RectList& character_bounds) override;
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
    virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefRequest> request,
                                bool user_gesture,
                                bool is_redirect) override;

    virtual void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                           TerminationStatus status,
                                           int error_code,
                                           const CefString& error_string) override;
#pragma endregion // CefRequestHandler

protected:
    static int                                  sBrowserCount;
    // List of any popup browser windows. Only accessed on the CEF UI thread.
    typedef std::list<CefRefPtr<CefBrowser>>    BrowserList;
    BrowserList                                 _popupBrowsers;
    int                                         _browserId{0};
    CefRefPtr<CefBrowser>                       _browser;
    bool                                        _isFocusOnEditableField{false};
    bool                                        _isClosing{false};
    CefViewClientDelegateBase::WeakPtr          _clientDelegate;
    //client::DropTargetHandle _dropTargetHandle;
    //CefRenderHandler::DragOperation _currentDragOp;
};
}

#endif //!CEFVIEWCLIENT_H
