/**
* @file        CefViewClientDelegateInterface.h
* @brief       CefViewClientDelegateInterface class declaration
* @version     1.0
* @author      heefuture
* @date        2025.09.04
* @copyright
*/
#ifndef CEFVIEWCLIENTDELEGATEINTERFACE_H
#define CEFVIEWCLIENTDELEGATEINTERFACE_H
#pragma once
#include <memory>
#include <include/cef_client.h>

namespace cefview {

// Interface for browser delegates. All BrowserDelegates must be returned via
// CreateBrowserDelegates. Do not perform work in the BrowserDelegate
// constructor. See CefBrowserProcessHandler for documentation.
class CefViewClientDelegateInterface {
public:
    typedef std::shared_ptr<CefViewClientDelegateInterface> RefPtr;
    typedef std::weak_ptr<CefViewClientDelegateInterface> WeakPtr;
    virtual ~CefViewClientDelegateInterface() {}

#pragma region CefClient
    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId sourceProcess,
                                          CefRefPtr<CefProcessMessage> message) { return false; }
#pragma endregion // CefClient

#pragma region CefContextMenuHandler
    // Called in non-UI thread
    virtual void onBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) = 0;

    // Called in non-UI thread
    virtual bool onContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefContextMenuParams> params,
                                      int commandId,
                                      CefContextMenuHandler::EventFlags eventFlags) = 0;
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
        virtual void onAddressChange(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, const CefString& url) = 0;

        virtual void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) = 0;

        virtual bool onCursorChange(CefRefPtr<CefBrowser> browser,
                                    CefCursorHandle cursor,
                                    cef_cursor_type_t type,
                                    const CefCursorInfo& customCursorInfo) = 0;

        virtual bool onConsoleMessage(CefRefPtr<CefBrowser> browser,
                                      cef_log_severity_t level,
                                      const CefString& message,
                                      const CefString& source,
                                      int line) = 0;
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
    virtual bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefDownloadItem> downloadItem,
                                  const CefString& suggestedName,
                                  CefRefPtr<CefBeforeDownloadCallback> callback) = 0;

    virtual void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefDownloadItem> downloadItem,
                                   CefRefPtr<CefDownloadItemCallback> callback) = 0;
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
    virtual bool onDragEnter(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> dragData,
                             CefRenderHandler::DragOperationsMask mask) = 0;
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
    virtual bool onKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle osEvent) = 0;
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
    // Called in non-UI thread
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
                               bool *noJavascriptAccess) = 0;

    virtual void onAfterCreated(CefRefPtr<CefBrowser> browser) = 0;

    virtual void onBeforeClose(CefRefPtr<CefBrowser> browser) = 0;
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
    virtual void onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) = 0;

    virtual void onLoadStart(CefRefPtr<CefBrowser> browser,	CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transitionType) = 0;

    virtual void onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) = 0;

    virtual void onLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefLoadHandler::ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl) = 0;
#pragma endregion // CefLoadHandler

#pragma region CefRenderHandler
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
                               CefRefPtr<CefDragData> dragData,
                               CefRenderHandler::DragOperationsMask allowedOps,
                               int x, int y) = 0;

    virtual void updateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation) = 0;

    virtual void onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
                                              const CefRange& selectionRange,
                                              const CefRenderHandler::RectList& characterBounds) = 0;
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
    virtual bool onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool userGesture, bool isRedirect) = 0;

    virtual void onRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                           CefRequestHandler::TerminationStatus status,
                                           int errorCode,
                                           const CefString& errorString) = 0;
#pragma endregion // CefRequestHandler
    // Wrapper for JS and C++ interaction functionality
    //virtual bool onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser) = 0;
    //virtual bool onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string) = 0;
};

}  // namespace cefview

#endif //!CEFVIEWCLIENTDELEGATEINTERFACE_H
