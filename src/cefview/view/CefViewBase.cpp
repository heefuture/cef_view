#pragma clang diagnostic ignored "-Wunused-lambda-capture"

#include "CefViewBase.h"

using namespace cef;

CefViewBase::CefViewBase()
{
}

CefViewBase::~CefViewBase()
{
    _taskListAfterCreated.clear();
    _cefHandler.reset();
    _delegates.clear();
    _listeners.clear();
}

void CefViewBase::loadURL(const std::string& url)
{
    if (_cefHandler && _cefHandler->GetBrowser()) {
        CefRefPtr<CefFrame> frame = _cefHandler->GetBrowser()->GetMainFrame();
        if (!frame)
            return;

        frame->LoadURL(url);
    }
    else {
        std::function<void(void)> load_url_task = [this, url]() {
            if (_cefHandler.get() && _cefHandler->GetBrowser())
            {
                CefRefPtr<CefFrame> frame = _cefHandler->GetBrowser()->GetMainFrame();
                if (frame)
                {
                    frame->LoadURL(url);
                }
            }
        };
        _taskListAfterCreated.push_back(load_url_task);
    }
}

void CefViewBase::resize(int width, int height)
{
    if (_cefHandler && _cefHandler->GetBrowser()) {
        _cefHandler->NotifyRectUpdated();
    }
    else
    {
        std::function<void(void)> resize_task = [this, width, height]() {
            if (_cefHandler.get() && _cefHandler->GetBrowser()) {
                _cefHandler->NotifyRectUpdated();
            }
        };
        _taskListAfterCreated.push_back(resize_task);
    }
}

void CefViewBase::goBack()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->GoBack();
    }
}

void CefViewBase::goForward()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->GoForward();
    }
}

bool CefViewBase::canGoBack()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->CanGoBack();
    }
    return false;
}

bool CefViewBase::canGoForward()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->CanGoForward();
    }
    return false;
}

void CefViewBase::refresh()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->Reload();
    }
}

void CefViewBase::stopLoad()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->StopLoad();
    }
}

bool CefViewBase::isLoading()
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        return _cefHandler->GetBrowser()->IsLoading();
    }
    return false;
}

void CefViewBase::startDownload(const std::string& url)
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        _cefHandler->GetBrowser()->GetHost()->StartDownload(url);
    }
}

void CefViewBase::setZoomLevel(float zoom_level)
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        _cefHandler->GetBrowser()->GetHost()->SetZoomLevel(zoom_level);
    }
}

CefWindowHandle CefViewBase::getWindowHandle() const
{
    if (_cefHandler.get() && _cefHandler->GetBrowserHost().get())
        return _cefHandler->GetBrowserHost()->GetWindowHandle();

    return NULL;
}

void CefViewBase::registerProcessMessageHandler(ProcessMessageHandler* handler)
{
    if (handler) {
        CefRefPtr<ProcessMessageDelegateWrapper> delegateWrapper = new ProcessMessageDelegateWrapper(handler);
        _cefHandler->RegisterProcessMessageDelegates(delegateWrapper);
    }
}

bool CefViewBase::openDevTools()
{
    if (devtool_opened_)
        return true;

    auto browser = _cefHandler->GetBrowser();
    if (browser) {
        CefWindowInfo windowInfo;
        windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
        windowInfo.SetAsWindowless(nullptr);
        CefBrowserSettings settings;
        browser->GetHost()->ShowDevTools(windowInfo, _cefHandler, settings, CefPoint());
        devtool_opened_ = true;
    }
    return true;
}

void CefViewBase::closeDevTools()
{
    if (!devtool_opened_)
        return;
    auto browser = _cefHandler->GetBrowser();
    if (browser != nullptr)
    {
        browser->GetHost()->CloseDevTools();
        devtool_opened_ = false;
    }
}

void CefViewBase::addListener(std::shared_ptr<CefViewListener> listener)
{
    if (listener) {
        _listeners.push_back(listener);
    }
}

void CefViewBase::onPaint(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList &dirtyRects,
        const void *buffer,
        int width,
        int height)
{
    return;
}

void CefViewBase::onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::PaintElementType type,
        const CefRenderHandler::RectList &dirtyRects,
        const CefAcceleratedPaintInfo &info)
{
    return;
}

bool CefViewBase::getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    return false;
}

void CefViewBase::getViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    return;
}

bool CefViewBase::getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) {
    return false;
}

bool CefViewBase::getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) {
    return false;
}

void CefViewBase::onPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    return;
}

void CefViewBase::onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    return;
}

void CefViewBase::onBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
    return;
}

bool CefViewBase::onContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, CefContextMenuHandler::EventFlags event_flags)
{
    return false;
}

void CefViewBase::onAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    if (frame->IsMain())
    {
        auto id = browser->GetIdentifier();
        auto old_url = _url;
        _url = frame->GetURL();
        for (const auto& listener : _listeners) {
            if (listener) {
                listener->onUrlChange(id, old_url.ToString(), _url.ToString());
            }
        }
    }
}

void CefViewBase::onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    for (const auto &listener : _listeners) {
        if (listener) {
            listener->onTitleChange(browser->GetIdentifier(), title.ToString());
        }
    }
}

bool CefViewBase::onCursorChange(CefRefPtr<CefBrowser> browser,
    CefCursorHandle cursor,
    cef_cursor_type_t type,
    const CefCursorInfo &custom_cursor_info)
{
    return false;
}

void CefViewBase::onLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    auto id = browser->GetIdentifier();
    for (const auto &listener : _listeners) {
        if (listener) {
            listener->onLoadingStateChange(id, isLoading, canGoBack, canGoForward);
        }
    }
}

void CefViewBase::onLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::TransitionType transition_type)
{
    std::string url = frame->GetURL().ToString();
    for (const auto &listener : _listeners) {
        if (listener) {
            listener->onLoadStart(url);
        }
    }
}

void CefViewBase::onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    std::string url = frame->GetURL().ToString();
    for (const auto &listener : _listeners) {
        if (listener) {
            listener->onLoadEnd(url);
        }
    }
}

void CefViewBase::onLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefLoadHandler::ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    int browserId = browser->GetIdentifier();
    for (const auto &listener : _listeners) {
        if (listener) {
            listener->onLoadError(browserId, errorText.ToString(), failedUrl.ToString());
        }
    }
}

bool CefViewBase::onBeforePopup(CefRefPtr<CefBrowser> browser,
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

bool CefViewBase::onAfterCreated(CefRefPtr<CefBrowser> browser)
{
    if (browser) {
        for (const auto &listener : _listeners) {
            if (listener) {
                listener->onAfterCreated(browser->GetIdentifier());
            }
        }

        for (const auto &task : _taskListAfterCreated) {
            if (task) {
                task();
            }
        }
    }
    return false;
}

void CefViewBase::onBeforeClose(CefRefPtr<CefBrowser> browser)
{
    if (browser) {
        for (const auto &listener : _listeners) {
            if (listener) {
                listener->onBeforeClose(browser->GetIdentifier());
            }
        }
    }
}

bool CefViewBase::onBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{
    return false;
}

void CefViewBase::onRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int error_code, const CefString& error_string)
{
    return;
}

bool CefViewBase::onBeforeDownload(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        const CefString &suggested_name,
        CefRefPtr<CefBeforeDownloadCallback> callback)
{
    return false;
}

void CefViewBase::onDownloadUpdated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDownloadItem> download_item,
        CefRefPtr<CefDownloadItemCallback> callback)
{
}

// bool CefViewBase::onExecuteCppFunc(const CefString& function_name, const CefString& params, int js_callback_id, CefRefPtr<CefBrowser> browser)
// {
//     if (js_bridge_.get())
//     {
//         js_callback_thread_id_ = nbase::FrameworkThread::GetManagedThreadId();
//         return js_bridge_->ExecuteCppFunc(function_name, params, js_callback_id, browser);
//     }

//     return false;
// }

// bool CefViewBase::onExecuteCppCallbackFunc(int cpp_callback_id, const CefString& json_string)
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
