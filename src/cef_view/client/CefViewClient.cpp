#include "CefViewClient.h"

#include <utils/util.h>

#include <include/cef_app.h>
#include <include/cef_frame.h>
#include <include/base/cef_bind.h>
#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>

#include "CefSwitches.h"

namespace cefview{

int CefViewClient::sBrowserCount = 0;

CefViewClient::CefViewClient(CefViewClientDelegateInterface::RefPtr delegate)
    : _clientDelegate(delegate)
{
}

CefViewClient::~CefViewClient()
{
}
// void CefViewClient::NotifyRectUpdated()
// {
//     if (!CefCurrentlyOn(TID_UI)) {
//         CefPostTask(TID_UI, base::BindOnce(&CefViewClient::NotifyRectUpdated, this));
//         return;
//     }

//     // 调用WasResized接口，调用后，BrowserHandler会调用GetViewRect接口来获取浏览器对象新的位置
//     if (_browser.get() && _browser->GetHost().get())
//         _browser->GetHost()->WasResized();
// }

CefRefPtr<CefBrowserHost> CefViewClient::GetBrowserHost()
{
    if (_browser.get())
    {
        return _browser->GetHost();
    }
    return nullptr;
}

void CefViewClient::CloseAllBrowser()
{
    class CloseAllBrowserTask : public CefTask
    {
        IMPLEMENT_REFCOUNTING(CloseAllBrowserTask);
    public:
        CloseAllBrowserTask(const std::list<CefRefPtr<CefBrowser>>& browserList) {
            browserList_.assign(browserList.begin(), browserList.end());
        }
    public:
        void Execute() override
        {
            for (auto it : browserList_) {
                if (it != nullptr)
                    it->GetHost()->CloseBrowser(true);
            }
        }
    private:
        std::list<CefRefPtr<CefBrowser>> browserList_;
    };
    std::list<CefRefPtr<CefBrowser>> tempList(_popupBrowsers);
    if (_browser) {
        tempList.push_back(_browser);
    }
    CefPostTask(TID_UI, new CloseAllBrowserTask(tempList));
}

#pragma region CefClient
bool CefViewClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    // Check for messages from the client renderer.
    std::string message_name = message->GetName();
    if (message_name == kFocusedNodeChangedMessage) {
        // A message is sent from ClientRenderDelegate to tell us whether the
        // currently focused DOM node is editable. Use of |m_bFocusOnEditableField|
        // is redundant with CefKeyEvent.focus_on_editable_field in OnPreKeyEvent
        // but is useful for demonstration purposes.
        _isFocusOnEditableField = message->GetArgumentList()->GetBool(0);
        return true;
    }

    bool handled = false;
    if (auto clientDelegate = _clientDelegate.lock()) {
        handled = clientDelegate->onProcessMessageReceived(browser, frame, source_process, message);
    }

    // // Execute delegate callbacks.
    // ProcessMessageDelegateSet::iterator it = _processMessageDelegates.begin();
    // for (; it != _processMessageDelegates.end() && !handled; ++it) {
    //     handled = (*it)->onProcessMessageReceived(this, browser, frame, source_process,
    //                                           message);
    // }

    return handled;
}
#pragma endregion // CefClient

#pragma region CefContextMenuHandler
void CefViewClient::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
CefRefPtr<CefFrame> frame,
CefRefPtr<CefContextMenuParams> params,
CefRefPtr<CefMenuModel> model)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onBeforeContextMenu(browser, frame, params, model);
    }
    else {
        // Customize the context menu...
        if ((params->GetTypeFlags() & (CM_TYPEFLAG_PAGE | CM_TYPEFLAG_FRAME)) != 0) {
            if (model->GetCount() > 0) {
                // 禁止右键菜单
                model->Clear();
            }
        }
    }
}

bool CefViewClient::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->onContextMenuCommand(browser, frame, params, command_id, event_flags);
    }
    else {
        return false;
    }
}
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
// CefDisplayHandler methods
void CefViewClient::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    if (!CefCurrentlyOn(TID_UI)) {
        CefPostTask(TID_UI, base::BindOnce(&CefViewClient::OnAddressChange, this, browser, frame, url));
        return;
    }

    if (_browserId == browser->GetIdentifier() && frame->IsMain()) {
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onAddressChange(browser, frame, url);
        }
    }
}

void CefViewClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    if (!CefCurrentlyOn(TID_UI)) {
        CefPostTask(TID_UI, base::BindOnce(&CefViewClient::OnTitleChange, this, browser, title));
        return;
    }
    if (_browserId == browser->GetIdentifier()) {
        // Update the browser window title...
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onTitleChange(browser, title);
        }
    }
}

bool CefViewClient::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo &custom_cursor_info)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->onCursorChange(browser, cursor, type, custom_cursor_info);
    }
    return false;
}


bool CefViewClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line)
{
    // Log a console message...
    return false;
}
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
bool CefViewClient::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback)
{
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->onBeforeDownload(browser, download_item, suggested_name, callback);
    }

    return false;
}

void CefViewClient::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback)
{
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onDownloadUpdated(browser, download_item, callback);
    }
}
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
bool CefViewClient::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefDragData> drag_data,
                                CefRenderHandler::DragOperationsMask mask)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->onDragEnter(browser, drag_data, mask);
    }
    return false;
}
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
bool CefViewClient::OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->onKeyEvent(browser, event, os_event);
    }
    return false;
}
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
// CefLifeSpanHandler methods
bool CefViewClient::OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
    REQUIRE_UI_THREAD();
    bool bRet = false;
    auto clientDelegate = _clientDelegate.lock();
    if (_browser && !target_url.empty() && clientDelegate) {
        // 返回true则继续在控件内打开新链接，false则弹窗打开
        bRet = clientDelegate->onBeforePopup(_browser, frame, popup_id, target_url, target_frame_name, target_disposition, user_gesture, popupFeatures, windowInfo, client, settings, extra_info, no_javascript_access);
        if (bRet) {
            _browser->GetMainFrame()->LoadURL(target_url);
        }
    }

    if (!bRet) {
        // 获取当前主窗口的尺寸
        CefRect rect;
        GetViewRect(browser, rect);

        // 如果获取到的 ViewRect 不合理，使用默认尺寸
        if (rect.width <= 100 || rect.height <= 100) {
            rect.width = 100;
            rect.height = 100;
        }

        // 只设置窗口尺寸，不改变其他属性
        windowInfo.bounds.width = rect.width;
        windowInfo.bounds.height = rect.height;

        // 使用相同的客户端处理器
        client = this;
    }

    return bRet; // 返回true则不创建新窗口，false则创建新窗口
}

void CefViewClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();

    if (!_browser)   {
        // We need to keep the main child window, but not popup windows
        _browser = browser;
        _browserId = browser->GetIdentifier();
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onAfterCreated(browser);
        }
    } else if (browser->IsPopup())
    {
        // Add to the list of popup browsers.
        _popupBrowsers.push_back(browser);
    }
    else //load_new_window
    {
        //
    }
    sBrowserCount++;
}

bool CefViewClient::DoClose(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();
    if (_browser->IsSame(browser)) {
        // Set a flag to indicate that the window close should be allowed.
        _isClosing = true;
    }
    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void CefViewClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();

    if (_browserId == browser->GetIdentifier()) {
        // Free the browser pointer so that the browser can be destroyed
        _browser.reset();
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onBeforeClose(browser);
        }
    } else if (browser->IsPopup()) {
        // Remove from the browser popup list.
        BrowserList::iterator bit = _popupBrowsers.begin();
        for (; bit != _popupBrowsers.end(); ++bit) {
            if ((*bit)->IsSame(browser)) {
                _popupBrowsers.erase(bit);
                break;
            }
        }
    }
    else {
    }

    if (--sBrowserCount == 0) {
        // All browser windows have closed.
        // Remove and delete message router handlers.
        // _processMessageDelegates.clear();
        CefQuitMessageLoop();
    }
}
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
void CefViewClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    REQUIRE_UI_THREAD();
    // Update UI for browser state...
    if (_browserId == browser->GetIdentifier()) {
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onLoadingStateChange(browser, isLoading, canGoBack, canGoForward);
        }
    }
}

void CefViewClient::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
    REQUIRE_UI_THREAD();
    // A frame has started loading content...
    if (_browserId == browser->GetIdentifier() && frame->IsMain()) {
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onLoadStart(browser, frame, transition_type);
        }
    }
}

void CefViewClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    REQUIRE_UI_THREAD();
    // A frame has finished loading content...
    if (_browserId == browser->GetIdentifier() && frame->IsMain()) {
        if (auto clientDelegate = _clientDelegate.lock()) {
            clientDelegate->onLoadEnd(browser, frame, httpStatusCode);
        }
    }
}

void CefViewClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    REQUIRE_UI_THREAD();
    if (errorCode == ERR_ABORTED)
        return;
    // A frame has failed to load content...
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onLoadError(browser, frame, errorCode, errorText, failedUrl);
    }
}
#pragma endregion // CefLoadHandler

#pragma region CefRenderHandler
// CefRenderHandler methods
bool CefViewClient::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->getRootScreenRect(browser, rect);
    }
    return false;
}

void CefViewClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    REQUIRE_UI_THREAD();
    // Never return an empty rectangle.
    rect.x = rect.y = 0;
    rect.width = rect.height = 1;

    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->getViewRect(browser, rect);
    }
}

bool CefViewClient::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->getScreenPoint(browser, viewX, viewY, screenX, screenY);
    }
    return false;
}

bool CefViewClient::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->getScreenInfo(browser, screenInfo);
    }
    return false;
}

void CefViewClient::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onPopupShow(browser, show);
    }
}

void CefViewClient::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onPopupSize(browser, rect);
    }
}

void CefViewClient::OnPaint(CefRefPtr<CefBrowser> browser,
    PaintElementType type,
    const RectList& dirtyRects,
    const void* buffer,
    int width,
    int height)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onPaint(browser, type, dirtyRects, buffer, width, height);
    }
}

void CefViewClient::OnAcceleratedPaint(CefRefPtr<CefBrowser> browser,
    PaintElementType type,
    const RectList &dirtyRects,
    const CefAcceleratedPaintInfo &info)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onAcceleratedPaint(browser, type, dirtyRects, info);
    }
}

bool CefViewClient::StartDragging(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDragData> drag_data,
        CefRenderHandler::DragOperationsMask allowed_ops,
        int x, int y)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->startDragging(browser, drag_data, allowed_ops, x, y);
    }
    return false;
}

void CefViewClient::UpdateDragCursor(CefRefPtr<CefBrowser> browser,
        CefRenderHandler::DragOperation operation)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->updateDragCursor(browser, operation);
    }
}

void CefViewClient::OnImeCompositionRangeChanged(
        CefRefPtr<CefBrowser> browser,
        const CefRange& selection_range,
        const CefRenderHandler::RectList& character_bounds)
{
    REQUIRE_UI_THREAD();
    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onImeCompositionRangeChanged(browser, selection_range, character_bounds);
    }
}
#pragma endregion // CefRenderHandler

#pragma region CefRequestHandler
bool CefViewClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{
    if (auto clientDelegate = _clientDelegate.lock()) {
        return clientDelegate->onBeforeBrowse(browser, frame, request, user_gesture, is_redirect);
    }

    return false;
}

void CefViewClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int error_code, const CefString& error_string)
{
    if (!CefCurrentlyOn(TID_UI)) {
        // 把操作跳转到Cef线程执行
        CefPostTask(TID_UI, base::BindOnce(&CefViewClient::OnRenderProcessTerminated, this, browser, status, error_code, error_string));
        return;
    }

    if (auto clientDelegate = _clientDelegate.lock()) {
        clientDelegate->onRenderProcessTerminated(browser, status, error_code, error_string);
    }
}
#pragma endregion // CefRequestHandler

} // namespace cefview
