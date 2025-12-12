#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif

#include "CefViewClientDelegate.h"
#include "CefWebView.h"

#include "include/cef_parser.h"

#include <bridge/CefJsBridgeBrowser.h>
#include <client/CefSwitches.h>
#include <utils/WinUtil.h>

using namespace cefview;

// Returns a data: URI with the specified contents.
static std::string GetDataURI(const std::string& data, const std::string& mimeType)
{
    return "data:" + mimeType + ";base64," +
        CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
        .ToString();
}

CefViewClientDelegate::CefViewClientDelegate(CefWebView* view)
    : _view(view)
{
    assert(_view);
    _jsBridgeBrowser = std::make_shared<CefJsBridgeBrowser>();
}

CefViewClientDelegate::~CefViewClientDelegate()
{
}

// void CefViewClientDelegate::registerProcessMessageHandler(ProcessMessageHandler* handler)
// {
//     if (handler) {
//         CefRefPtr<ProcessMessageDelegateWrapper> delegateWrapper = new ProcessMessageDelegateWrapper(handler);
//         _cefViewCLient->RegisterProcessMessageDelegates(delegateWrapper);
//     }
// }

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
#pragma endregion // CefClient



#pragma region CefContextMenuHandler
void CefViewClientDelegate::onBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                             CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefContextMenuParams> params,
                                             CefRefPtr<CefMenuModel> model)
{
    return;
}

bool CefViewClientDelegate::onContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                              CefRefPtr<CefFrame> frame,
                                              CefRefPtr<CefContextMenuParams> params,
                                              int commandId,
                                              CefContextMenuHandler::EventFlags eventFlags)
{
    return false;
}
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
void CefViewClientDelegate::onAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    if (frame->IsMain())
    {
        auto id = browser->GetIdentifier();
        auto old_url = _url;
        _url = frame->GetURL();

        _view->onUrlChange(id, old_url.ToString(), _url.ToString());
    }
}

void CefViewClientDelegate::onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    _view->onTitleChange(browser->GetIdentifier(), title.ToString());
}

bool CefViewClientDelegate::onCursorChange(CefRefPtr<CefBrowser> browser,
                                        CefCursorHandle cursor,
                                        cef_cursor_type_t type,
                                        const CefCursorInfo &customCursorInfo)
{
    return _view->onCursorChange(browser, cursor, type, customCursorInfo);
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
                                          const CefString &suggestedName,
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

    // Handle shortcuts before browser processes the key event
    return _view->handleShortcutKey(event.windows_key_code, event.modifiers);
}

bool CefViewClientDelegate::onKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle osEvent)
{
    return false;
}
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
bool CefViewClientDelegate::onBeforePopup(CefRefPtr<CefBrowser> browser,
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
                                       bool *noJavascriptAccess)
{
    return false;
}

void CefViewClientDelegate::onAfterCreated(CefRefPtr<CefBrowser> browser)
{
    _view->onAfterCreated(browser->GetIdentifier());
}

void CefViewClientDelegate::onBeforeClose(CefRefPtr<CefBrowser> browser)
{
    _view->onBeforeClose(browser->GetIdentifier());
}
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
void CefViewClientDelegate::onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    auto id = browser->GetIdentifier();
    _view->onLoadingStateChange(id, isLoading, canGoBack, canGoForward);
}

void CefViewClientDelegate::onLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transitionType)
{
    std::string url = frame->GetURL().ToString();
    _view->onLoadStart(url);
}

void CefViewClientDelegate::onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    std::string url = frame->GetURL().ToString();
    _view->onLoadEnd(url);
}

void CefViewClientDelegate::onLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    std::string url = failedUrl.ToString();
    _view->onLoadError(browser->GetIdentifier(), url, errorText.ToString());

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
        const CefRenderHandler::RectList &dirtyRects,
        const void *buffer,
        int width,
        int height)
{
    _view->onPaint(type, dirtyRects, buffer, width, height);
}

void CefViewClientDelegate::onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList &dirtyRects,
        const CefAcceleratedPaintInfo &info)
{
    _view->onAcceleratedPaint(type, dirtyRects, info);
}

bool CefViewClientDelegate::getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    return _view->getRootScreenRect(rect);
}

void CefViewClientDelegate::getViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    _view->getViewRect(rect);
}

bool CefViewClientDelegate::getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) {
    return _view->getScreenPoint(viewX, viewY, screenX, screenY);
}

bool CefViewClientDelegate::getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) {
    return _view->getScreenInfo(screenInfo);
}

void CefViewClientDelegate::onPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    return;
}

void CefViewClientDelegate::onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    return;
}

bool CefViewClientDelegate::startDragging(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDragData> dragData,
    CefRenderHandler::DragOperationsMask allowedOps,
    int x, int y)
{
    return _view->startDragging(dragData, allowedOps, x, y);
}

void CefViewClientDelegate::updateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation)
{
    _view->updateDragCursor(operation);
}

void CefViewClientDelegate::onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
    const CefRange& selectionRange,
    const CefRenderHandler::RectList& characterBounds)
{
    _view->onImeCompositionRangeChanged(selectionRange, characterBounds);
}
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
bool CefViewClientDelegate::onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool userGesture, bool isRedirect)
{
    return false;
}

void CefViewClientDelegate::onRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int errorCode, const CefString& errorString)
{
    return;
}
#pragma endregion // CefRequestHandler

// bool CefViewClientDelegate::onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser)
// {
//     if (js_bridge_.get())
//     {
//         js_callback_thread_id_ = nbase::FrameworkThread::GetManagedThreadId();
//         return js_bridge_->ExecuteCppFunc(function_name, params, js_callback_id, browser);
//     }

//     return false;
// }

// bool CefViewClientDelegate::onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string)
// {
//     if (js_bridge_.get())
//     {
//         if (js_callback_thread_id_ != -1)
//         {
//             nbase::ThreadManager::PostTask(js_callback_thread_id_, [this, cpp_callback_id, json_string]
//             {
//                 js_bridge_->ExecuteCppCallbackFunc(cpp_callback_id, json_string);
//             });
//         }
//         else
//         {
//             return js_bridge_->ExecuteCppCallbackFunc(cpp_callback_id, json_string);
//         }

//     }

//     return false;
// }
