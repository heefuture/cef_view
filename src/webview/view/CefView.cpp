#include "CefView.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
//#include "include/cef_runnable.h"
//#include "cef_control/handler/browser_handler.h"
//#include "cef_control/manager/cef_manager.h"

namespace cef {

CefView::CefView(HWND hwnd) : _hwnd(hwnd)
{

}

CefView::~CefView(void)
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        // Request that the main browser close.
        _cefHandler->GetBrowserHost()->CloseBrowser(true);
        //_cefHandler->SetHostWindow(NULL);
        _cefHandler->SetHandlerDelegate(NULL);
    }
}

void CefView::init()
{
    if (_cefHandler.get() == nullptr)
    {
        LONG style = GetWindowLong(_hwnd, GWL_STYLE);
        SetWindowLong(_hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        //ASSERT((GetWindowExStyle(_hwnd) & WS_EX_LAYERED) == 0 && L"无法在分层窗口内使用本控件");

        _cefHandler = new CefHandler;
        //_cefHandler->SetHostWindow(_hwnd);
        _cefHandler->SetHandlerDelegate(this);
        // reCreateBrowser();
    }

    // if (!js_bridge_.get())
    // {
    //     js_bridge_.reset(new CefJSBridge);
    // }
}

void CefView::reCreateBrowser(int width, int height)
{
    if (_cefHandler.get() == nullptr)
        return;

    // 如果浏览器已经存在，则先关闭它
    if (_cefHandler->GetBrowser() != nullptr)
    {
        _cefHandler->GetBrowserHost()->CloseBrowser(true);
        //_cefHandler->SetHostWindow(NULL);
        _cefHandler->SetHandlerDelegate(NULL);
    }

    // 创建新的浏览器
    CefWindowInfo window_info;
    CefRect cefrect; //(rect.left, rect.top, rect.right, rect.bottom);
    window_info.SetAsChild(_hwnd, cefrect);

    CefBrowserSettings browser_settings;
    CefBrowserHost::CreateBrowser(window_info, _cefHandler, L"", browser_settings, nullptr, nullptr);
}

void CefView::setRect(int left, int top, int width, int height)
{
    if (_hwnd) {
        SetWindowPos(_hwnd, HWND_TOP, left, top, width, height, SWP_NOZORDER);
    }
}

// void CefView::HandleMessage(EventArgs& event)
// {
//     if (_cefHandler.get() && _cefHandler->GetBrowser().get() == NULL)
//         return __super::HandleMessage(event);

//     else if (event.Type == kEventInternalSetFocus)
//     {
//         _cefHandler->GetBrowserHost()->SetFocus(true);
//     }
//     else if (event.Type == kEventInternalKillFocus)
//     {
//         _cefHandler->GetBrowserHost()->SetFocus(false);
//     }

//     __super::HandleMessage(event);
// }

void CefView::setVisible(bool bVisible /*= true*/)
{
    if (_hwnd) {
        if (bVisible) {
            ShowWindow(_hwnd, SW_SHOW);
        }
        else {
            SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        }
    }
}

bool CefView::getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    RECT window_rect = { 0 };
    HWND root_window = GetAncestor(_hwnd, GA_ROOT);
    if (::GetWindowRect(root_window, &window_rect)) {
        rect = CefRect(window_rect.left, window_rect.top,
                       window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);
        return true;
    }
    return false;
}

void CefView::getViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    RECT clientRect;
    if (!::GetClientRect(_hwnd, &clientRect)) return ;

    rect.x = rect.y = 0;
    rect.width = clientRect.right == 0 ? 1 : clientRect.right;
    rect.height = clientRect.bottom == 0 ? 1 : clientRect.bottom;
}

bool CefView::getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) {
    if (!::IsWindow(_hwnd)) return false;

    // Convert the point from view coordinates to actual screen coordinates.
    POINT screen_pt = { viewX, viewY };
    ClientToScreen(_hwnd, &screen_pt);
    screenX = screen_pt.x;
    screenY = screen_pt.y;
    return true;
}

bool CefView::getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) {

    return false;
}


}