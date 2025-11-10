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

CefWebView::CefWebView(const std::string& url, const CefWebViewSetting& settings, HWND parentHwnd)
{
    if (parentHwnd == nullptr) {
        return;
    } else {
        _parentHwnd = parentHwnd;
    }

    CefRect rect;
    if (settings.width > 0 && settings.height > 0) {
        rect.set(settings.x, settings.y, settings.width, settings.height);
    }
    else {
        CefRect rect = getWindowRect(parentHwnd);
    }

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

    LONG style = ::GetWindowLong(_hwnd, GWL_STYLE);
    ::SetWindowLong(_hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

    // init();
    createCefBrowser();
}

CefWebView::~CefWebView()
{
    if (_clientDelegate && _clientDelegate->GetBrowser().get())
    {
        // Request that the main browser close.
        _clientDelegate->GetBrowser()->GetHost()->CloseBrowser(true);
        _clientDelegate.reset();
    }
    _taskListAfterCreated.clear();
    destroy();
}

void CefWebView::init()
{
    if (!_clientDelegate)
    {
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

void CefWebView::createCefBrowser(const std::string& url, const CefWebViewSetting& settings)
{
    if (_clientDelegate) return;

    _clientDelegate = std::make_shared<CefViewClientDelegate>(std::shared_from_this());
    _client = new CefViewClient(_clientDelegate);

    // 创建新的浏览器
    CefWindowInfo windowInfo;
    CefRect cefRect = getWindowRect(_hwnd);
    cefRect.x = cefRect.y = 0; // 确保坐标从0开始
    windowInfo.SetAsChild(_hwnd, cefRect);

    CefBrowserSettings browser_settings;
    CefBrowserHost::CreateBrowser(windowInfo, _client, CefString(url), browser_settings, nullptr, nullptr);
}

void CefWebView::setRect(int left, int top, int width, int height)
{
    if (_hwnd) {
        ::SetWindowPos(_hwnd, nullptr, left, top, width, height, SWP_NOZORDER);
    }
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

void CefWebView::loadUrl(const std::string& url)
{
    if (_cefViewCLient && _cefViewCLient->GetBrowser()) {
        CefRefPtr<CefFrame> frame = _cefViewCLient->GetBrowser()->GetMainFrame();
        if (!frame)
            return;

        frame->LoadURL(url);
        _url = url;
    }
    else {
        std::function<void(void)> loadUrlTask = [this, url]() {
            if (_cefViewCLient.get() && _cefViewCLient->GetBrowser())
            {
                CefRefPtr<CefFrame> frame = _cefViewCLient->GetBrowser()->GetMainFrame();
                if (frame) {
                    frame->LoadURL(url);
                    _url = url;
                }
            }
        };
        _taskListAfterCreated.push_back(loadUrlTask);
    }
}

const std::string& CefWebView::getUrl() const
{
    return _url;
}

void CefWebView::goBack()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->GoBack();
    }
}

void CefWebView::goForward()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->GoForward();
    }
}

bool CefWebView::canGoBack()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        return browser->CanGoBack();
    }
    return false;
}

bool CefWebView::canGoForward()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        return browser->CanGoForward();
    }
    return false;
}

void CefWebView::refresh()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->Reload();
    }
}

void CefWebView::stopLoad()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->StopLoad();
    }
}

bool CefWebView::isLoading()
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        return browser->IsLoading();
    }
    return false;
}

void CefWebView::startDownload(const std::string& url)
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->GetHost()->StartDownload(url);
    }
}

void CefWebView::setZoomLevel(float zoom_level)
{
    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->GetHost()->SetZoomLevel(zoom_level);
    }
}

CefWindowHandle CefWebView::getWindowHandle() const
{
    if (auto auto browser = _cefViewCLient->GetBrowser()) {
        return browser->GetHost()->GetWindowHandle();
    }
    return NULL;
}

// void CefWebView::registerProcessMessageHandler(ProcessMessageHandler* handler)
// {
//     if (handler) {
//         CefRefPtr<ProcessMessageDelegateWrapper> delegateWrapper = new ProcessMessageDelegateWrapper(handler);
//         _cefViewCLient->RegisterProcessMessageDelegates(delegateWrapper);
//     }
// }

bool CefWebView::openDevTools()
{
    if (_isDevToolsOpened) return true;

    if (auto browser = _cefViewCLient->GetBrowser()) {
        CefWindowInfo windowInfo;
        windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
        windowInfo.SetAsWindowless(nullptr);
        CefBrowserSettings settings;
        browser->GetHost()->ShowDevTools(windowInfo, _cefViewCLient, settings, CefPoint());
        _isDevToolsOpened = true;
    }
    return _isDevToolsOpened;
}

void CefWebView::closeDevTools()
{
    if (!_isDevToolsOpened)
        return;

    if (auto browser = _cefViewCLient->GetBrowser()) {
        browser->GetHost()->CloseDevTools();
        _isDevToolsOpened = false;
    }
}

void CefWebView::evaluateJavaScript(const std::string& script)
{
    if (auto browser = _clientDelegate->getCefBrowser()) {
        CefRefPtr<CefFrame> frame = browser->GetMainFrame();
        if (frame) {
            frame->ExecuteJavaScript(CefString(script), frame->GetURL(), 0);
        }
    }
}

}