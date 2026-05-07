#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif

#import "CefViewClientDelegate.h"

#include "include/cef_parser.h"

#include "bridge/CefJsBridgeBrowser.h"
#include "utils/CefSwitches.h"

using namespace cefview;

// Returns a data: URI with the specified contents.
static std::string GetDataURI(const std::string& data, const std::string& mimeType)
{
    return "data:" + mimeType + ";base64," +
        CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

CefViewClientDelegate::CefViewClientDelegate(id<CefWebViewObserver> observer)
    : _observer(observer)
{
    _jsBridgeBrowser = std::make_shared<CefJsBridgeBrowser>();
}

CefViewClientDelegate::~CefViewClientDelegate()
{
    _observer = nil;
}

#pragma region CefClient
bool CefViewClientDelegate::onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                                     CefRefPtr<CefFrame> frame,
                                                     CefProcessId sourceProcess,
                                                     CefRefPtr<CefProcessMessage> message)
{
    std::string msgName = message->GetName();
    if (msgName == kCallCppFunctionMessage) {
        CefString funcName = message->GetArgumentList()->GetString(0);
        CefString param = message->GetArgumentList()->GetString(1);
        int jsCallbackId = message->GetArgumentList()->GetInt(2);

        if (_jsBridgeBrowser) {
            _jsBridgeBrowser->executeCppFunc(funcName, param, jsCallbackId, browser);
        }

        return true;
    } else if (msgName == kExecuteCppCallbackMessage) {
        CefString param = message->GetArgumentList()->GetString(0);
        int callbackId = message->GetArgumentList()->GetInt(1);

        if (_jsBridgeBrowser) {
            _jsBridgeBrowser->executeCppCallbackFunc(callbackId, param);
        }

        return true;
    }

    return false;
}

void CefViewClientDelegate::onEditableFocusChanged(CefRefPtr<CefProcessMessage> message)
{
    if (_observer) {
        [_observer onEditableFocusChanged:message];
    }
}
#pragma endregion // CefClient

#pragma region CefContextMenuHandler
void CefViewClientDelegate::onBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                                CefRefPtr<CefFrame> frame,
                                                CefRefPtr<CefContextMenuParams> params,
                                                CefRefPtr<CefMenuModel> model)
{
#ifdef _DEBUG
    // In debug mode, add DevTools menu item
    if (model->GetCount() > 0) {
        model->AddSeparator();
    }

    constexpr int kShowDevToolsMenuId = MENU_ID_USER_FIRST + 1;
    model->AddItem(kShowDevToolsMenuId, "Open DevTools");
#else
    // Keep default context menu enabled.
#endif
}

bool CefViewClientDelegate::onContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                                 CefRefPtr<CefFrame> frame,
                                                 CefRefPtr<CefContextMenuParams> params,
                                                 int commandId,
                                                 CefContextMenuHandler::EventFlags eventFlags)
{
#ifdef _DEBUG
    constexpr int kShowDevToolsMenuId = MENU_ID_USER_FIRST + 1;
    if (commandId == kShowDevToolsMenuId) {
        if (_observer && [_observer respondsToSelector:@selector(openDevTools)]) {
            [_observer openDevTools];
        }
        return true;
    }
#endif
    return false;
}
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
void CefViewClientDelegate::onAddressChange(CefRefPtr<CefBrowser> browser,
                                            CefRefPtr<CefFrame> frame,
                                            const CefString& url)
{
    if (frame->IsMain()) {
        auto id = browser->GetIdentifier();
        auto oldUrl = _url;
        _url = frame->GetURL();

        if (_observer && [_observer respondsToSelector:@selector(onUrlChangeWithBrowserId:oldUrl:url:)]) {
            [_observer onUrlChangeWithBrowserId:id oldUrl:oldUrl.ToString() url:_url.ToString()];
        }
    }
}

void CefViewClientDelegate::onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    if (_observer && [_observer respondsToSelector:@selector(onTitleChangeWithBrowserId:title:)]) {
        [_observer onTitleChangeWithBrowserId:browser->GetIdentifier() title:title.ToString()];
    }
}

bool CefViewClientDelegate::onCursorChange(CefRefPtr<CefBrowser> browser,
                                           CefCursorHandle cursor,
                                           cef_cursor_type_t type,
                                           const CefCursorInfo& customCursorInfo)
{
    if (_observer) {
        return [_observer onCursorChangeWithBrowser:browser cursor:cursor type:type customCursorInfo:customCursorInfo];
    }
    return false;
}

bool CefViewClientDelegate::onConsoleMessage(CefRefPtr<CefBrowser> browser,
                                             cef_log_severity_t level,
                                             const CefString& message,
                                             const CefString& source,
                                             int line)
{
    return false;
}
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
bool CefViewClientDelegate::onBeforeDownload(CefRefPtr<CefBrowser> browser,
                                             CefRefPtr<CefDownloadItem> downloadItem,
                                             const CefString& suggestedName,
                                             CefRefPtr<CefBeforeDownloadCallback> callback)
{
    return false;
}

void CefViewClientDelegate::onDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                              CefRefPtr<CefDownloadItem> downloadItem,
                                              CefRefPtr<CefDownloadItemCallback> callback)
{
}
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
bool CefViewClientDelegate::onDragEnter(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefDragData> dragData,
                                        CefRenderHandler::DragOperationsMask mask)
{
    return false;
}
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
bool CefViewClientDelegate::onPreKeyEvent(CefRefPtr<CefBrowser> browser,
                                          const CefKeyEvent& event,
                                          CefEventHandle osEvent,
                                          bool* isKeyboardShortcut)
{
    // Only handle key down events
    if (event.type != KEYEVENT_RAWKEYDOWN && event.type != KEYEVENT_KEYDOWN) {
        return false;
    }

    if (_observer) {
        return [_observer handleShortcutKeyWithKeyCode:event.native_key_code modifiers:event.modifiers];
    }
    return false;
}

bool CefViewClientDelegate::onKeyEvent(CefRefPtr<CefBrowser> browser,
                                       const CefKeyEvent& event,
                                       CefEventHandle osEvent)
{
    return false;
}
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
bool CefViewClientDelegate::onBeforePopup(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          int popupId,
                                          const CefString& targetUrl,
                                          const CefString& targetFrameName,
                                          CefLifeSpanHandler::WindowOpenDisposition targetDisposition,
                                          bool userGesture,
                                          const CefPopupFeatures& popupFeatures,
                                          CefWindowInfo& windowInfo,
                                          CefRefPtr<CefClient>& client,
                                          CefBrowserSettings& settings,
                                          CefRefPtr<CefDictionaryValue>& extraInfo,
                                          bool* noJavascriptAccess)
{
    return false;
}

void CefViewClientDelegate::onAfterCreated(CefRefPtr<CefBrowser> browser)
{
    if (_observer && [_observer respondsToSelector:@selector(onAfterCreatedWithBrowserId:)]) {
        [_observer onAfterCreatedWithBrowserId:browser->GetIdentifier()];
    }
}

void CefViewClientDelegate::onBeforeClose(CefRefPtr<CefBrowser> browser)
{
    if (_observer && [_observer respondsToSelector:@selector(onBeforeCloseWithBrowserId:)]) {
        [_observer onBeforeCloseWithBrowserId:browser->GetIdentifier()];
    }
}
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
void CefViewClientDelegate::onLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                                 bool isLoading,
                                                 bool canGoBack,
                                                 bool canGoForward)
{
    auto id = browser->GetIdentifier();
    if (_observer && [_observer respondsToSelector:@selector(onLoadingStateChangeWithBrowserId:isLoading:canGoBack:canGoForward:)]) {
        [_observer onLoadingStateChangeWithBrowserId:id
                                       isLoading:isLoading
                                       canGoBack:canGoBack
                                    canGoForward:canGoForward];
    }
}

void CefViewClientDelegate::onLoadStart(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefLoadHandler::TransitionType transitionType)
{
    std::string url = frame->GetURL().ToString();
    if (_observer && [_observer respondsToSelector:@selector(onLoadStartWithUrl:)]) {
        [_observer onLoadStartWithUrl:url];
    }
}

void CefViewClientDelegate::onLoadEnd(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      int httpStatusCode)
{
    std::string url = frame->GetURL().ToString();
    if (_observer && [_observer respondsToSelector:@selector(onLoadEndWithUrl:)]) {
        [_observer onLoadEndWithUrl:url];
    }
}

void CefViewClientDelegate::onLoadError(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        CefLoadHandler::ErrorCode errorCode,
                                        const CefString& errorText,
                                        const CefString& failedUrl)
{
    std::string url = failedUrl.ToString();
    if (_observer && [_observer respondsToSelector:@selector(onLoadErrorWithBrowserId:errorText:failedUrl:)]) {
        [_observer onLoadErrorWithBrowserId:browser->GetIdentifier()
                              errorText:errorText.ToString()
                              failedUrl:url];
    }

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED) {
        return;
    }

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
          "<h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText)
       << " (" << errorCode << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}
#pragma endregion // CefLoadHandler

#pragma region CefRenderHandler
void CefViewClientDelegate::onPaint(CefRefPtr<CefBrowser> browser,
                                    CefRenderHandler::PaintElementType type,
                                    const CefRenderHandler::RectList& dirtyRects,
                                    const void* buffer,
                                    int width,
                                    int height)
{
    if (_observer) {
        [_observer onPaintWithType:type dirtyRects:dirtyRects buffer:buffer width:width height:height];
    }
}

void CefViewClientDelegate::onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
                                               CefRenderHandler::PaintElementType type,
                                               const CefRenderHandler::RectList& dirtyRects,
                                               const CefAcceleratedPaintInfo& info)
{
    if (_observer) {
        [_observer onAcceleratedPaintWithType:type dirtyRects:dirtyRects info:info];
    }
}

bool CefViewClientDelegate::getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    if (_observer) {
        return [_observer getRootScreenRect:rect];
    }
    return false;
}

void CefViewClientDelegate::getViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    if (_observer) {
        [_observer getViewRect:rect];
    }
}

bool CefViewClientDelegate::getScreenPoint(CefRefPtr<CefBrowser> browser,
                                           int viewX, int viewY,
                                           int& screenX, int& screenY)
{
    if (_observer) {
        return [_observer getScreenPointWithViewX:viewX viewY:viewY screenX:&screenX screenY:&screenY];
    }
    return false;
}

bool CefViewClientDelegate::getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screenInfo)
{
    if (_observer) {
        return [_observer getScreenInfo:screenInfo];
    }
    return false;
}

void CefViewClientDelegate::onPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
}

void CefViewClientDelegate::onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
}

bool CefViewClientDelegate::startDragging(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefDragData> dragData,
                                          CefRenderHandler::DragOperationsMask allowedOps,
                                          int x, int y)
{
    if (_observer) {
        return [_observer startDraggingWithDragData:dragData allowedOps:allowedOps x:x y:y];
    }
    return false;
}

void CefViewClientDelegate::updateDragCursor(CefRefPtr<CefBrowser> browser,
                                             CefRenderHandler::DragOperation operation)
{
    if (_observer) {
        [_observer updateDragCursor:operation];
    }
}

void CefViewClientDelegate::onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
                                                         const CefRange& selectionRange,
                                                         const CefRenderHandler::RectList& characterBounds)
{
    if (_observer) {
        [_observer onImeCompositionRangeChangedWithRange:selectionRange characterBounds:characterBounds];
    }
}
#pragma endregion // CefRenderHandler

#pragma region CefPermissionHandler
bool CefViewClientDelegate::onShowPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                                   uint64_t promptId,
                                                   const CefString& requestingOrigin,
                                                   uint32_t requestedPermissions,
                                                   CefRefPtr<CefPermissionPromptCallback> callback)
{
    // Auto-grant clipboard permissions
    if (requestedPermissions & CEF_PERMISSION_TYPE_CLIPBOARD) {
        callback->Continue(CEF_PERMISSION_RESULT_ACCEPT);
        return true;
    }

    return false;
}
#pragma endregion // CefPermissionHandler

#pragma region CefRequestHandler
bool CefViewClientDelegate::onBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                           CefRefPtr<CefFrame> frame,
                                           CefRefPtr<CefRequest> request,
                                           bool userGesture,
                                           bool isRedirect)
{
    return false;
}

void CefViewClientDelegate::onRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                                      CefRequestHandler::TerminationStatus status,
                                                      int errorCode,
                                                      const CefString& errorString)
{
}
#pragma endregion // CefRequestHandler
