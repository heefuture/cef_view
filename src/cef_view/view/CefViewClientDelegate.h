/**
* @file        CefViewClientDelegate.h
* @brief       CefViewClientDelegate 类的定义
*              用于处理 CefHandler 浏览器的基本操作和事件
* @version     1.0
* @author      heefuture
* @date        2025.07.08
* @copyright
*/
#ifndef CEFVIEWCLIENTDELEGATE_H
#define CEFVIEWCLIENTDELEGATE_H
#pragma once
#include <client/CefViewClient.h>
#include <client/CefViewClientDelegateInterface.h>
// #include <view/ProcessMessageDelegateWrapper.h>

#include "include/cef_base.h"

#include <memory>
#include <functional>

namespace cefview {
class CefWebView;

class CefViewClientDelegate : public CefViewClientDelegateInterface
{
public:
    CefViewClientDelegate(CefWebView* view);
    ~CefViewClientDelegate();

public:
    // /**
    // * @brief 注册一个 ProcessMessageHandler 对象，主要用来处理js消息
    // * @param [in] handler ProcessMessageHandler 对象指针
    // */
    // virtual void registerProcessMessageHandler(ProcessMessageHandler* handler);


    // virtual bool onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser) override;

    // virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string) override;

private:
#pragma region CefClient
    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId sourceProcess,
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
                                      int commandId,
                                      CefContextMenuHandler::EventFlags eventFlags) override;
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
    virtual void onAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url) override;

    virtual void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) override;

    virtual bool onCursorChange(CefRefPtr<CefBrowser> browser,
                                CefCursorHandle cursor,
                                cef_cursor_type_t type,
                                const CefCursorInfo &customCursorInfo) override;

    virtual bool onConsoleMessage(CefRefPtr<CefBrowser> browser,
                                  cef_log_severity_t level,
                                  const CefString& message,
                                  const CefString& source,
                                  int line) override;
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
    virtual bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> downloadItem,
        const CefString &suggestedName,
        CefRefPtr<CefBeforeDownloadCallback> callback) override;

    virtual void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> downloadItem,
        CefRefPtr<CefDownloadItemCallback> callback) override;
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
    virtual bool onDragEnter(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> dragData,
                             CefRenderHandler::DragOperationsMask mask) override;
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
    virtual bool onKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle osEvent) override;
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
    virtual bool onBeforePopup(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               int popupId,
                               const CefString &targetUrl,
                               const CefString &targetFrameName,
                               CefLifeSpanHandler::WindowOpenDisposition targetDisposition,
                               bool userGesture,
                               const CefPopupFeatures &popupFeatures,
                               CefWindowInfo &windowInfo,
                               CefRefPtr<CefClient> &client,
                               CefBrowserSettings &settings,
                               CefRefPtr<CefDictionaryValue> &extraInfo,
                               bool *noJavascriptAccess) override;

    virtual void onAfterCreated(CefRefPtr<CefBrowser> browser) override;

    virtual void onBeforeClose(CefRefPtr<CefBrowser> browser) override;
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
    virtual void onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override;

    virtual void onLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transitionType) override;

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
        CefRefPtr<CefDragData> dragData,
        CefRenderHandler::DragOperationsMask allowedOps,
        int x, int y) override;

    virtual void updateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) override;

    virtual void onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
        const CefRange& selectionRange,
        const CefRenderHandler::RectList& characterBounds) override;
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
    virtual bool onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool userGesture, bool isRedirect) override;

    virtual void onRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int errorCode, const CefString& errorString) override;
#pragma endregion // CefRequestHandler

    //// 封装一些 JS 与 C++ 交互的功能
    //virtual bool onExecuteCppFunc(const CefString &function_name, const CefString &params, int js_callback_id, CefRefPtr<CefBrowser> browser) override;

    //virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString &json_string) override;
    //enum WebState { kNone, kCreating, kCreated };
    //WebState _webstate;
protected:
    CefWebView*                             _view{nullptr};
    // std::vector<CefRefPtr<ProcessMessageDelegateWrapper>> _delegates;
    //std::shared_ptr<CefJSBridge> js_bridge_;
    CefString                               _url;
    bool                                    _isDevtoolsOpened{false};
    //int                         js_callback_thread_id_ = -1; // 保存接收到 JS 调用 CPP 函数的代码所属线程，以后触发 JS 回调时把回调转到那个线程
};
}

#endif //!CEFVIEWCLIENTDELEGATE_H
