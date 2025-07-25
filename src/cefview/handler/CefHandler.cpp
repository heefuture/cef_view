#include "CefHandler.h"

#include <util/util.h>

#include "include/cef_app.h"
#include "include/cef_frame.h"
#include "include/base/cef_bind.h"
#include "include/base/cef_callback.h"

#include "include/wrapper/cef_closure_task.h"
using namespace cef;

int CefHandler::s_browser_count = 0;

CefHandler::CefHandler() {
}

void CefHandler::NotifyRectUpdated()
{
    if (!CefCurrentlyOn(TID_UI)) {
        // 把操作跳转到Cef线程执行
        CefPostTask(TID_UI, base::BindOnce(&CefHandler::NotifyRectUpdated, this));
        return;
    }

    // 调用WasResized接口，调用后，BrowserHandler会调用GetViewRect接口来获取浏览器对象新的位置
    if (browser_.get() && browser_->GetHost().get())
        browser_->GetHost()->WasResized();
}

CefRefPtr<CefBrowserHost> CefHandler::GetBrowserHost()
{
    if (browser_.get())
    {
        return browser_->GetHost();
    }
    return nullptr;
}

void CefHandler::CloseAllBrowser()
{
    class CloseAllBrowserTask : public CefTask
    {
        IMPLEMENT_REFCOUNTING(CloseAllBrowserTask);
    public:
        CloseAllBrowserTask(const std::list<CefRefPtr<CefBrowser>>& browser_list)
        {
            browser_list_.assign(browser_list.begin(), browser_list.end());
        }
    public:
        void Execute() override
        {
            for (auto it : browser_list_) {
                if (it != nullptr)
                    it->GetHost()->CloseBrowser(true);
            }
        }
    private:
        std::list<CefRefPtr<CefBrowser>> browser_list_;
    };
    std::list<CefRefPtr<CefBrowser>> temp_list(popup_browsers_);
    if (browser_) {
        temp_list.push_back(browser_);
    }
    CefPostTask(TID_UI, new CloseAllBrowserTask(temp_list));
}

bool CefHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    // Check for messages from the client renderer.
    std::string message_name = message->GetName();
    if (message_name == "FocusedNodeChanged") {
        // A message is sent from ClientRenderDelegate to tell us whether the
        // currently focused DOM node is editable. Use of |m_bFocusOnEditableField|
        // is redundant with CefKeyEvent.focus_on_editable_field in OnPreKeyEvent
        // but is useful for demonstration purposes.
        is_focus_oneditable_field_ = message->GetArgumentList()->GetBool(0);
        return true;
    }

    bool handled = false;

    // Execute delegate callbacks.
    ProcessMessageDelegateSet::iterator it = process_message_delegates_.begin();
    for (; it != process_message_delegates_.end() && !handled; ++it) {
        handled = (*it)->onProcessMessageReceived(this, browser, frame, source_process,
                                              message);
    }

    return handled;
}

#pragma region CefLifeSpanHandler
// CefLifeSpanHandler methods
bool CefHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
    if (browser_ && !target_url.empty() && handle_delegate_) {
        // 返回true则继续在控件内打开新链接，false则弹窗打开
        bRet = handle_delegate_->onBeforePopup(browser, frame, popup_id, target_url, target_frame_name, target_disposition, user_gesture, popupFeatures, windowInfo, client, settings, extra_info, no_javascript_access);
        if (bRet) {
            browser_->GetMainFrame()->LoadURL(target_url);
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

void CefHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();

    if (!browser_)   {
        // We need to keep the main child window, but not popup windows
        browser_ = browser;
        browser_id_ = browser->GetIdentifier();
        if (handle_delegate_) {
            handle_delegate_->onAfterCreated(browser);
        }
    } else if (browser->IsPopup())
    {
        // Add to the list of popup browsers.
        popup_browsers_.push_back(browser);
    }
    else //load_new_window
    {
        //
    }
    s_browser_count++;
}

bool CefHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();
    if (browser_->IsSame(browser)) {
        // Set a flag to indicate that the window close should be allowed.
        is_closing_ = true;
    }
    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void CefHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    REQUIRE_UI_THREAD();

    if (browser_id_ == browser->GetIdentifier()) {
        // Free the browser pointer so that the browser can be destroyed
        browser_.reset();
        if (handle_delegate_) {
            handle_delegate_->onBeforeClose(browser);
        }
    } else if (browser->IsPopup()) {
        // Remove from the browser popup list.
        BrowserList::iterator bit = popup_browsers_.begin();
        for (; bit != popup_browsers_.end(); ++bit) {
            if ((*bit)->IsSame(browser)) {
                popup_browsers_.erase(bit);
                break;
            }
        }
    }
    else {
    }

    if (--s_browser_count == 0) {
        // All browser windows have closed.
        // Remove and delete message router handlers.
        process_message_delegates_.clear();
        CefQuitMessageLoop();
    }
}
#pragma endregion

#pragma region CefRenderHandler
// CefRenderHandler methods
bool CefHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        return handle_delegate_->getRootScreenRect(browser, rect);
    }
    return false;
}

void CefHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    REQUIRE_UI_THREAD();
    // Never return an empty rectangle.
    rect.x = rect.y = 0;
    rect.width = rect.height = 1;

    if (handle_delegate_) {
        handle_delegate_->getViewRect(browser, rect);
    }
}

bool CefHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        return handle_delegate_->getScreenPoint(browser, viewX, viewY, screenX, screenY);
    }
    return false;
}

bool CefHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        return handle_delegate_->getScreenInfo(browser, screenInfo);
    }
    return false;
}

void CefHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        handle_delegate_->onPopupShow(browser, show);
    }
}

void CefHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        handle_delegate_->onPopupSize(browser, rect);
    }
}

void CefHandler::OnPaint(CefRefPtr<CefBrowser> browser,
    PaintElementType type,
    const RectList& dirtyRects,
    const void* buffer,
    int width,
    int height)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        handle_delegate_->onPaint(browser, type, dirtyRects, buffer, width, height);
    }
}

void CefHandler::OnAcceleratedPaint(CefRefPtr<CefBrowser> browser,
    PaintElementType type,
    const RectList &dirtyRects,
    const CefAcceleratedPaintInfo &info)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        handle_delegate_->onAcceleratedPaint(browser, type, dirtyRects, info);
    }
}

#pragma endregion

#pragma region CefContextMenuHandler
// CefContextMenuHandler methods
void CefHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
CefRefPtr<CefFrame> frame,
CefRefPtr<CefContextMenuParams> params,
CefRefPtr<CefMenuModel> model)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_)
    {
        handle_delegate_->onBeforeContextMenu(browser, frame, params, model);
    }
    else
    {
        // Customize the context menu...
        if ((params->GetTypeFlags() & (CM_TYPEFLAG_PAGE | CM_TYPEFLAG_FRAME)) != 0)
        {
            if (model->GetCount() > 0)
            {
                // 禁止右键菜单
                model->Clear();
            }
        }
    }
}

bool CefHandler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
    if (handle_delegate_)
        return handle_delegate_->onContextMenuCommand(browser, frame, params, command_id, event_flags);
    else
        return false;
}

#pragma endregion

#pragma region CefDisplayHandler

// CefDisplayHandler methods
void CefHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    if (!CefCurrentlyOn(TID_UI)) {
        // 把操作跳转到Cef线程执行
        CefPostTask(TID_UI, base::BindOnce(&CefHandler::OnAddressChange, this, browser, frame, url));
        return;
    }

    if (browser_id_ == browser->GetIdentifier() && frame->IsMain()) {
        if (handle_delegate_)
            handle_delegate_->onAddressChange(browser, frame, url);
    }
}

void CefHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    if (!CefCurrentlyOn(TID_UI)) {
        // 把操作跳转到Cef线程执行
        CefPostTask(TID_UI, base::BindOnce(&CefHandler::OnTitleChange, this, browser, title));
        return;
    }
    if (browser_id_ == browser->GetIdentifier()) {
        // Update the browser window title...
        if (handle_delegate_)
            handle_delegate_->onTitleChange(browser, title);
    }
}

bool CefHandler::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo &custom_cursor_info)
{
    REQUIRE_UI_THREAD();
    if (handle_delegate_) {
        return handle_delegate_->onCursorChange(browser, cursor, type, custom_cursor_info);
    }
    return false;
}

bool CefHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line)
{
    // Log a console message...
    return false;
}
#pragma endregion

// CefLoadHandler methods
void CefHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    REQUIRE_UI_THREAD();
    // Update UI for browser state...
    if (browser_id_ == browser->GetIdentifier()) {
        if (handle_delegate_) {
            handle_delegate_->onLoadingStateChange(browser, isLoading, canGoBack, canGoForward);
        }
    }
}

void CefHandler::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
    REQUIRE_UI_THREAD();
    // A frame has started loading content...
    if (browser_id_ == browser->GetIdentifier() && frame->IsMain()) {
        if (handle_delegate_) {
            handle_delegate_->onLoadStart(browser, frame, transition_type);
        }
    }
}

void CefHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    REQUIRE_UI_THREAD();
    // A frame has finished loading content...
    if (browser_id_ == browser->GetIdentifier() && frame->IsMain()) {
        if (handle_delegate_) {
            handle_delegate_->onLoadEnd(browser, frame, httpStatusCode);
        }
    }
}

void CefHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    REQUIRE_UI_THREAD();
    if (errorCode == ERR_ABORTED)
        return;
    // A frame has failed to load content...
    if (handle_delegate_) {
        handle_delegate_->onLoadError(browser, frame, errorCode, errorText, failedUrl);
    }
}

// CefRequestHandler methods
bool CefHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{
    if (handle_delegate_)
        return handle_delegate_->onBeforeBrowse(browser, frame, request, user_gesture, is_redirect);

    return false;
}

void CefHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status, int error_code, const CefString& error_string)
{
    if (!CefCurrentlyOn(TID_UI)) {
        // 把操作跳转到Cef线程执行
        CefPostTask(TID_UI, base::BindOnce(&CefHandler::OnRenderProcessTerminated, this, browser, status, error_code, error_string));
        return;
    }

    if (handle_delegate_)
        handle_delegate_->onRenderProcessTerminated(browser, status, error_code, error_string);
}

bool CefHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    const CefString& suggested_name,
    CefRefPtr<CefBeforeDownloadCallback> callback)
{
    if (handle_delegate_)
        return handle_delegate_->onBeforeDownload(browser, download_item, suggested_name, callback);

    return false;
}

void CefHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDownloadItem> download_item,
    CefRefPtr<CefDownloadItemCallback> callback)
{
    if (handle_delegate_)
        handle_delegate_->onDownloadUpdated(browser, download_item, callback);
}
