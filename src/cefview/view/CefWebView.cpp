#include "CefWebView.h"
#include <utils/util_win.h>
#include "include/cef_browser.h"
#include "include/cef_frame.h"

#include <sstream>

namespace cefview {

typedef void (*SubWindowRepaintCallback)(void*);
// struct SubWindowUserData {
//     SubWindowRepaintCallback repaint_callback;
//     void* repaint_callback_param;
// };

static LRESULT CALLBACK subWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // if (uMsg == WM_PAINT) {
    //     auto user_data =
    //         (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    //     if (user_data && user_data->repaint_callback) {
    //         user_data->repaint_callback(user_data->repaint_callback_param);
    //     }
    // } else if (uMsg == WM_NCDESTROY) {
    //     SubWindowUserData* user_data =
    //         (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    //     delete user_data;
    // }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CefWebView::CefWebView(HWND parentHwnd)
{
    if (parentHwnd == nullptr) {
        return;
    } else {
        _parentHwnd = parentHwnd;
    }

    CefRect rect = getWindowRect(parentHwnd);
    if (rect.IsEmpty()) {
        return;
    }

    _hwnd = createSubWindow(parentHwnd, rect.x, rect.y, rect.width, rect.height);
    if (_hwnd == nullptr) {
        return;
    }

    // auto style = ::GetWindowLongPtrW(parentHwnd, GWL_STYLE);
    // style |= WS_CHILD;
    // ::SetWindowLongPtrW(_hwnd, GWL_STYLE, style);
    // ::SetParent(_hwnd, _parentHwnd);
    // ::SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    // ::SetWindowPos(_hwnd, nullptr, 0, 0, rect.width, rect.height, SWP_NOMOVE);
    init();
}

CefWebView::~CefWebView(void)
{
    if (_cefHandler.get() && _cefHandler->GetBrowser().get())
    {
        // Request that the main browser close.
        _cefHandler->GetBrowserHost()->CloseBrowser(true);
        //_cefHandler->SetHostWindow(NULL);
        _cefHandler->SetHandlerDelegate(NULL);
    }
}

void CefWebView::init()
{
    if (_cefHandler.get() == nullptr)
    {
        LONG style = ::GetWindowLong(_hwnd, GWL_STYLE);
        ::SetWindowLong(_hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        //ASSERT((GetWindowExStyle(_hwnd) & WS_EX_LAYERED) == 0 && L"无法在分层窗口内使用本控件");

        _cefHandler = new CefHandler;
        //_cefHandler->SetHostWindow(_hwnd);
        _cefHandler->SetHandlerDelegate(this);
        reCreateBrowser();
    }

    // if (!js_bridge_.get())
    // {
    //     js_bridge_.reset(new CefJSBridge);
    // }
}


HWND CefWebView::createSubWindow(HWND parentHwnd, int x, int y,int width, int height, bool showWindow /*= true*/)
{
    //static std::string name = "CefWindow";
    //// Use a new window class name for ever window to ensure that a new window can be created
    //// even if the last one was not properly destroyed
    //static size_t windowIdx = 0;
    //std::ostringstream nameStream;
    //nameStream << name << "_" << windowIdx++;
    // _className = nameStream.str();

    static const wchar_t className[] = L"CefWindowClass";
    WNDCLASS wc = {};
    if (!::GetClassInfo(::GetModuleHandle(nullptr), className, &wc)) {
        wc.style =  CS_OWNDC | CS_HREDRAW | CS_VREDRAW;// redraw if size changes
        wc.lpfnWndProc = &subWindowProc;               // points to window procedure
        wc.cbWndExtra = sizeof(void*) ;                // save extra window memory
        wc.lpszClassName = className;                  // name of window class
        ::RegisterClass(&wc);
    }
    HWND ret = ::CreateWindowEx(
                        WS_EX_NOPARENTNOTIFY,  // do not bother our parent window
                        className,
                        L"CefWindow",
                        WS_CHILD,
                        0,0,width,height,
                        parentHwnd,
                        nullptr,
                        nullptr,
                        nullptr);

    // auto user_data = new SubWindowUserData();
    // user_data->repaint_callback = repaint_callback;
    // user_data->repaint_callback_param = repaint_callback_param;
    // SetWindowLongPtr(ret, GWLP_USERDATA, (LONG_PTR)user_data);
    if (showWindow)
        ::ShowWindow(ret, SW_SHOW);
    return ret;
}

void CefWebView::destroy() {
    if (_hwnd) {
        ::DestroyWindow(_hwnd);
        _hwnd = 0;
    }
    //UnregisterClassA(_className.c_str(), nullptr);
}

void CefWebView::reCreateBrowser()
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
    CefRect cefrect = getWindowRect(_hwnd);
    cefrect.x = cefrect.y = 0; // 确保坐标从0开始
    window_info.SetAsChild(_hwnd, cefrect);

    CefBrowserSettings browser_settings;
    CefBrowserHost::CreateBrowser(window_info, _cefHandler, L"", browser_settings, nullptr, nullptr);
}

void CefWebView::setRect(int left, int top, int width, int height)
{
    if (_hwnd) {
        ::SetWindowPos(_hwnd, nullptr, left, top, width, height, SWP_NOZORDER);
    }
    _clientDelegate->resize(width, height);
}

void CefWebView::setVisible(bool bVisible /*= true*/)
{
    if (_hwnd) {
        if (bVisible) {
            ::ShowWindow(_hwnd, SW_SHOW);
        }
        else {
            ::ShowWindow(_hwnd, SW_HIDE);
            //::SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        }
    }
}

}