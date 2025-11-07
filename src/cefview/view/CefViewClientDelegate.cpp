#pragma clang diagnostic ignored "-Wunused-lambda-capture"

#include "CefWebView.h"
#include "CefViewClientDelegate.h"

#include <utils/util_win.h>

using namespace cefview;

CefViewClientDelegate::CefViewClientDelegate(HWND hwnd)
    : _hwnd(hwnd)
{
}

CefViewClientDelegate::~CefViewClientDelegate()
{
    _taskListAfterCreated.clear();
    _cefViewCLient.reset();
}

CefRefPtr<CefBrowser> CefViewClientDelegate::getCefBrowser() const
{
    if (_cefViewCLient)
        return _cefViewCLient->GetBrowser();
    return nullptr;
}

void CefViewClientDelegate::loadUrl(const std::string& url)
{
    if (_cefViewCLient && _cefViewCLient->GetBrowser()) {
        CefRefPtr<CefFrame> frame = _cefViewCLient->GetBrowser()->GetMainFrame();
        if (!frame)
            return;

        frame->LoadURL(url);
    }
    else {
        std::function<void(void)> loadUrlTask = [this, url]() {
            if (_cefViewCLient.get() && _cefViewCLient->GetBrowser())
            {
                CefRefPtr<CefFrame> frame = _cefViewCLient->GetBrowser()->GetMainFrame();
                if (frame)
                {
                    frame->LoadURL(url);
                }
            }
        };
        _taskListAfterCreated.push_back(loadUrlTask);
    }
}

const std::string& CefViewClientDelegate::getUrl() const
{
    return _url;
}

void CefViewClientDelegate::resize(int width, int height)
{
    if (_cefViewCLient && _cefViewCLient->GetBrowser()) {
        _cefViewCLient->NotifyRectUpdated();
    }
    else
    {
        std::function<void(void)> resizeTask = [this, width, height]() {
            if (_cefViewCLient.get() && _cefViewCLient->GetBrowser()) {
                _cefViewCLient->NotifyRectUpdated();
            }
        };
        _taskListAfterCreated.push_back(resizeTask);
    }
}

// void CefViewClientDelegate::registerProcessMessageHandler(ProcessMessageHandler* handler)
// {
//     if (handler) {
//         CefRefPtr<ProcessMessageDelegateWrapper> delegateWrapper = new ProcessMessageDelegateWrapper(handler);
//         _cefViewCLient->RegisterProcessMessageDelegates(delegateWrapper);
//     }
// }

bool CefViewClientDelegate::openDevTools()
{
    if (_isDevToolsOpened)
        return true;

    if (auto browser = _cefViewCLient->GetBrowser()) {
        CefWindowInfo windowInfo;
        windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
        windowInfo.SetAsWindowless(nullptr);
        CefBrowserSettings settings;
        browser->GetHost()->ShowDevTools(windowInfo, _cefViewCLient, settings, CefPoint());
        _isDevToolsOpened = true;
    }
    return true;
}

void CefViewClientDelegate::closeDevTools()
{
    if (!_isDevToolsOpened)
        return;

    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->GetHost()->CloseDevTools();
        _isDevToolsOpened = false;
    }
}

#pragma region CefClient
bool CefViewClientDelegate::onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                                     CefRefPtr<CefFrame> frame,
                                                     CefProcessId source_process,
                                                     CefRefPtr<CefProcessMessage> message)
{
    if (message && browser) {
        auto id = browser->GetIdentifier();
        auto name = message->GetName().ToString();
        auto args = message->GetArgumentList();
        std::string jsonArgs;
        if (args && args->GetSize() > 0) {
            jsonArgs = args->GetString(0).ToString();
        }

        if (auto view = _view.lock()) {
            view->onProcessMessageReceived(id, name, jsonArgs);
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
                                              int command_id,
                                              CefContextMenuHandler::EventFlags event_flags)
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

        if (auto view = _view.lock()) {
            view->onUrlChange(id, old_url.ToString(), _url.ToString());
        }
    }
}

void CefViewClientDelegate::onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    if (auto view = _view.lock()) {
        view->onTitleChange(browser->GetIdentifier(), title.ToString());
    }
}

bool CefViewClientDelegate::onCursorChange(CefRefPtr<CefBrowser> browser,
                                        CefCursorHandle cursor,
                                        cef_cursor_type_t type,
                                        const CefCursorInfo &custom_cursor_info)
{
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
                                          CefRefPtr<CefDownloadItem> download_item,
                                          const CefString &suggested_name,
                                          CefRefPtr<CefBeforeDownloadCallback> callback)
{
    return false;
}

void CefViewClientDelegate::onDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                           CefRefPtr<CefDownloadItem> download_item,
                                           CefRefPtr<CefDownloadItemCallback> callback)
{
}
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
bool CefViewClientDelegate::onDragEnter(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefDragData> drag_data,
                                     CefRenderHandler::DragOperationsMask mask)
{
    return false;
}
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
bool CefViewClientDelegate::OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event)
{
    return false;
}
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
bool CefViewClientDelegate::onBeforePopup(CefRefPtr<CefBrowser> browser,
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
                                       bool *no_javascript_access)
{
    return false;
}

bool CefViewClientDelegate::onAfterCreated(CefRefPtr<CefBrowser> browser)
{
    if (browser) {
        if (auto view = _view.lock()) {
            view->onAfterCreated(browser->GetIdentifier());
        }

        for (const auto &task : _taskListAfterCreated) {
            if (task) {
                task();
            }
        }
        _taskListAfterCreated.clear();
        return true;
    }
    return false;
}

void CefViewClientDelegate::onBeforeClose(CefRefPtr<CefBrowser> browser)
{
    if (browser) {
        if (auto view = _view.lock()) {
            view->onBeforeClose(browser->GetIdentifier());
        }
    }
}
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
void CefViewClientDelegate::onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    auto id = browser->GetIdentifier();
    if (auto view = _view.lock()) {
        view->onLoadingStateChange(id, isLoading, canGoBack, canGoForward);
    }
}

void CefViewClientDelegate::onLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transition_type)
{
    std::string url = frame->GetURL().ToString();
    if (auto view = _view.lock()) {
        view->onLoadStart(url);
    }
}

void CefViewClientDelegate::onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    std::string url = frame->GetURL().ToString();
    if (auto view = _view.lock()) {
        view->onLoadEnd(url);
    }
}

void CefViewClientDelegate::onLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    int browserId = browser->GetIdentifier();
    if (auto view = _view.lock()) {
        view->onLoadError(browserId, errorText.ToString(), failedUrl.ToString());
    }
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
    return;
}

void CefViewClientDelegate::onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList &dirtyRects,
        const CefAcceleratedPaintInfo &info)
{
    return;
}

bool CefViewClientDelegate::getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    RECT window_rect = { 0 };
    HWND root_window = GetAncestor(_hwnd, GA_ROOT);
    if (::GetWindowRect(root_window, &window_rect)) {
        rect = CefRect(window_rect.left, window_rect.top,
                       window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);
        return true;
    }
    return false;
}

void CefViewClientDelegate::getViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    rect.x = rect.y = 0;
    rect = getWindowRect(_hwnd);
    if (rect.IsEmpty()) {
        rect.width = 1;
        rect.height = 1;
    }
}

bool CefViewClientDelegate::getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) {
    if (!::IsWindow(_hwnd)) return false;

    // Convert the point from view coordinates to actual screen coordinates.
    POINT screen_pt = { viewX, viewY };
    ::ClientToScreen(_hwnd, &screen_pt);
    screenX = screen_pt.x;
    screenY = screen_pt.y;
    return true;
}

bool CefViewClientDelegate::getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) {
    return false;
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
    CefRefPtr<CefDragData> drag_data,
    CefRenderHandler::DragOperationsMask allowed_ops,
    int x, int y)
{
    return false;
}

void CefViewClientDelegate::updateDragCursor(CefRefPtr<CefBrowser> browser, CefRenderHandler::DragOperation operation)
{
    return;
}

void CefViewClientDelegate::onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
    const CefRange& selection_range,
    const CefRenderHandler::RectList& character_bounds)
{
    return;
}
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
bool CefViewClientDelegate::onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{
    return false;
}

void CefViewClientDelegate::onRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int error_code, const CefString& error_string)
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
