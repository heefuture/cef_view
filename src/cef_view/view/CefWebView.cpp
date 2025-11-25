#include "CefWebView.h"

#include <sstream>
#include <windowsx.h>

#include <utils/Util.h>
#include <utils/WinUtil.h>
#include <utils/GeometryUtil.h>

#include <osr/OsrDragdropEvents.h>
#include <osr/OsrDragdropWin.h>
#include <osr/OsrImeHandlerWin.h>
#include <osr/OsrRendererD3D11.h>

#include <client/CefViewClient.h>
#include "CefViewClientDelegate.h"


#include "include/cef_browser.h"
#include "include/cef_frame.h"
//#include "include/base/cef_bind.h"
//#include "include/base/cef_callback.h"
//#include "include/wrapper/cef_closure_task.h"

namespace cefview {

class OsrDragEventsImpl : public OsrDragEvents
{
public:
    OsrDragEventsImpl(CefWebView* webview) : _webview(webview) {}
    ~OsrDragEventsImpl() { _webview = nullptr; }

    CefBrowserHost::DragOperationsMask onDragEnter(CefRefPtr<CefDragData> drag_data, CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) override {
        return _webview->onDragEnter(drag_data, ev, effect);
    }

    CefBrowserHost::DragOperationsMask onDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) override {
        return _webview->onDragOver(ev, effect);
    }

    void onDragLeave() override {
        _webview->onDragLeave();
    }

    CefBrowserHost::DragOperationsMask onDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) override {
        return _webview->onDrop(ev, effect);
    }

private:
    CefWebView* _webview;
};

// static
LRESULT CALLBACK CefWebView::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CefWebView *webView = util::getUserDataPtr<CefWebView *>(hwnd);
    if (!webView){
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    switch (uMsg) {
    case WM_IME_SETCONTEXT:
        webView->onIMESetContext(uMsg, wParam, lParam);
        return 0;
    case WM_IME_STARTCOMPOSITION:
        webView->onIMEStartComposition();
        return 0;
    case WM_IME_COMPOSITION:
        webView->onIMEComposition(uMsg, wParam, lParam);
        return 0;
    case WM_IME_ENDCOMPOSITION:
        webView->onIMECancelCompositionEvent();
        // Let WTL call::DefWindowProc() and release its resources.
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSELEAVE:
    case WM_MOUSEWHEEL:
        webView->onMouseEvent(uMsg, wParam, lParam);
        break;
    case WM_SIZE:
        webView->onSize();
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        webView->onFocus(uMsg == WM_SETFOCUS);
        break;

    case WM_CAPTURECHANGED:
    case WM_CANCELMODE:
        webView->onCaptureLost();
        break;

    case WM_SYSCHAR:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
        webView->onKeyEvent(uMsg, wParam, lParam);
        break;

    case WM_PAINT:
        webView->onPaint();
        return 0;

    case WM_TOUCH:
        if (webView->onTouchEvent(uMsg, wParam, lParam)) {
            return 0;
        } break;

    case WM_NCDESTROY:
        // Clear the reference to view.
        util::setUserDataPtr(hwnd, nullptr);
        webView->_hwnd = nullptr;
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

CefWebView::CefWebView(const std::string& url, const CefWebViewSetting& settings, HWND parentHwnd)
  : _parentHwnd(parentHwnd)
  , _settings(settings)
{
    if (parentHwnd == nullptr) {
        return;
    }

    _deviceScaleFactor = util::getWindowScaleFactor(parentHwnd);

    if (settings.width <= 0 || settings.height <= 0) {
        ::GetWindowRect(parentHwnd, &_clientRect);

        _settings.x = 0;
        _settings.y = 0;
        _settings.width = _clientRect.right - _clientRect.left;
        _settings.height = _clientRect.bottom - _clientRect.top;
    }

    _hwnd = createSubWindow(_parentHwnd, _settings.x, _settings.y, _settings.width, _settings.height);
    if (_hwnd == nullptr) {
        return;
    }

    createCefBrowser(url, settings);

    if (settings.offScreenRenderingEnabled) {
        _osrRenderer = std::make_unique<OsrRendererD3D11>(_hwnd, _settings.width, _settings.height);
        if (!_osrRenderer->initialize()) {
            _osrRenderer.reset();
            return;
        }

        _dragEvents = std::make_shared<OsrDragEventsImpl>(this);
        _dropTarget = OsrDropTargetWin::Create(_dragEvents.get(), _hwnd);
        HRESULT registerRes = RegisterDragDrop(_hwnd, _dropTarget);

        _imeHandler = std::make_unique<OsrImeHandlerWin>(_hwnd);
        // Enable Touch Events if requested
        // if (client::MainContext::Get()->TouchEventsEnabled()) {
        //     RegisterTouchWindow(hwnd_, 0);
        // }
    }
}

CefWebView::~CefWebView()
{
    if (_client && _browser.get()) {
        // Request that the main browser close.
        _browser->GetHost()->CloseBrowser(true);
    }

    _clientDelegate.reset();
    _taskListAfterCreated.clear();

    destroy();
}

void CefWebView::init()
{
    if (!_clientDelegate)
    {
    }
}

void CefWebView::destroy() {
    if (_settings.offScreenRenderingEnabled) {
        // Revoke/delete the drag&drop handler.
        ::RevokeDragDrop(_hwnd);
        _dropTarget = nullptr;

        // Release D3D11 renderer
        if (_osrRenderer) {
            _osrRenderer->uninitialize();
            _osrRenderer.reset();
        }

        _imeHandler.reset();

        if (_hwnd) {
            ::DestroyWindow(_hwnd);
            _hwnd = 0;
        }
    }
    //UnregisterClassA(_className.c_str(), nullptr);
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

    //const HBRUSH backgroundBrush = CreateSolidBrush(
    //    RGB(CefColorGetR(_settings.backgroundColor), CefColorGetG(_settings.backgroundColor),
    //        CefColorGetB(_settings.backgroundColor)));
    static const wchar_t className[] = L"CefViewWindowClass";
    WNDCLASS wc = {};
    if (!::GetClassInfo(::GetModuleHandle(nullptr), className, &wc)) {
        wc.style = CS_HREDRAW | CS_VREDRAW; // redraw if size changes
        wc.lpfnWndProc = &windowProc;                   // points to window procedure
        wc.cbWndExtra = sizeof(void*) ;                 // save extra window memory
        wc.lpszClassName = className;                   // name of window class
        //wc.hbrBackground = backgroundBrush;
        ::RegisterClass(&wc);
    }

    DWORD exStyle = 0;
    if (GetWindowLongPtr(parentHwnd, GWL_EXSTYLE) & WS_EX_NOACTIVATE) {
        // Don't activate the browser window on creation.
        exStyle |= WS_EX_NOACTIVATE;
    }
    exStyle |= WS_EX_NOPARENTNOTIFY;
    exStyle |= WS_EX_ACCEPTFILES;
    //exStyle |= WS_EX_TRANSPARENT | WS_EX_LAYERED;

    HWND ret = ::CreateWindowEx(
                        exStyle,
                        className,
                        L"CefViewWindow",
                        //WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                        WS_CHILD | WS_VISIBLE,
                        x,y,width,height,
                        parentHwnd,
                        nullptr,
                        nullptr,
                        nullptr);

    util::setUserDataPtr(ret, this);
    //if (showWindow)
    //    ::ShowWindow(ret, SW_SHOW);
    return ret;
}

void CefWebView::createCefBrowser(const std::string& url, const CefWebViewSetting& settings)
{
    if (_clientDelegate) return;

    _clientDelegate = std::make_shared<CefViewClientDelegate>(this);
    _client = new CefViewClient(_clientDelegate);

    CefWindowInfo windowInfo;
    if (GetWindowLongPtr(_parentHwnd, GWL_EXSTYLE) & WS_EX_NOACTIVATE) {
        // Don't activate the browser window on creation.
        windowInfo.ex_style |= WS_EX_NOACTIVATE;
    }

    if (settings.offScreenRenderingEnabled) {
        // Off-screen rendering mode
        windowInfo.SetAsWindowless(_hwnd);
        windowInfo.shared_texture_enabled = true;
    }
    else {
        // Native window mode
        // CEF child window coordinates are relative to _hwnd, so fill the entire _hwnd area from (0,0)
        CefRect cefRect(0, 0, _settings.width, _settings.height);
        windowInfo.SetAsChild(_hwnd, cefRect);
    }
    windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

    CefBrowserSettings browserSettings;
    browserSettings.windowless_frame_rate = settings.windowlessFrameRate;
    CefBrowserHost::CreateBrowser(windowInfo, _client, CefString(url), browserSettings, nullptr, nullptr);
    // CefBrowserHost::CreateBrowserSync(windowInfo, _client, CefString(url), browserSettings, nullptr, nullptr);
}

void CefWebView::setRect(int left, int top, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }
    _settings.x = left;
    _settings.y = top;
    _settings.width = width;
    _settings.height = height;

    ::SetWindowPos(_hwnd, nullptr, left, top, width, height, SWP_NOZORDER | SWP_NOMOVE);

    // Update D3D11 renderer size for OSR mode
    if (_settings.offScreenRenderingEnabled && _osrRenderer) {
        _osrRenderer->resize(width, height);
    }

    // For native window mode, resize CEF's child window
    HWND cefHwnd = getWindowHandle();
    if (cefHwnd && cefHwnd != _hwnd) {
        // CEF child window coordinates are relative to _hwnd (not the main window)
        // So position is always (0, 0) and only size needs to be updated
        ::SetWindowPos(cefHwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
    }
}

void CefWebView::setVisible(bool bVisible /*= true*/)
{
    if (bVisible) {
        ::ShowWindow(_hwnd, SW_SHOW);
        ::UpdateWindow(_hwnd);
    }
    else {
        ::ShowWindow(_hwnd, SW_HIDE);
    }

    if (!_browser) return;

    //_browser->GetHost()->WasHidden(!bVisible);
    //_browser->GetHost()->SetFocus(bVisible);

    HWND hwnd = getWindowHandle();
    if (hwnd && hwnd != _hwnd) {
        int cmdShow = bVisible ? SW_SHOW : SW_HIDE;
        ::ShowWindow(hwnd, cmdShow);
        //if (!bVisible) {
        //    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        //}
        ::UpdateWindow(hwnd);
    }
}

void CefWebView::loadUrl(const std::string& url)
{
    if (_browser && _browser.get()) {
        CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
        if (!frame) return;

        frame->LoadURL(url);
        _settings.url = url;
    }
    else {
        std::function<void(void)> loadUrlTask = [this, url]() {
            if (_browser && _browser.get()) {
                CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
                if (frame) {
                    frame->LoadURL(url);
                    _settings.url = url;
                }
            }
        };
        _taskListAfterCreated.push_back(loadUrlTask);
    }
}

const std::string& CefWebView::getUrl() const
{
    return _settings.url;
}

void CefWebView::goBack()
{
    if (_browser && _browser.get()) {
        _browser->GoBack();
    }
}

void CefWebView::goForward()
{
    if (_browser && _browser.get()) {
        _browser->GoForward();
    }
}

bool CefWebView::canGoBack()
{
    if (_browser && _browser.get()) {
        return _browser->CanGoBack();
    }
    return false;
}

bool CefWebView::canGoForward()
{
    if (_browser && _browser.get()) {
        return _browser->CanGoForward();
    }
    return false;
}

void CefWebView::refresh()
{
    if (_browser && _browser.get()) {
        _browser->Reload();
    }
}

void CefWebView::stopLoad()
{
    if (_browser && _browser.get()) {
        _browser->StopLoad();
    }
}

bool CefWebView::isLoading()
{
    if (_browser && _browser.get()) {
        return _browser->IsLoading();
    }
    return false;
}

void CefWebView::startDownload(const std::string& url)
{
    if (_browser && _browser.get()) {
        _browser->GetHost()->StartDownload(url);
    }
}

void CefWebView::setZoomLevel(float zoom_level)
{
    if (_browser && _browser.get()) {
        _browser->GetHost()->SetZoomLevel(zoom_level);
    }
}

HWND CefWebView::getWindowHandle() const
{
    if (_browser && _browser.get()) {
        return _browser->GetHost()->GetWindowHandle();
    }
    return nullptr;
}

// void CefWebView::registerProcessMessageHandler(ProcessMessageHandler* handler)
// {
//     if (handler) {
//         CefRefPtr<ProcessMessageDelegateWrapper> delegateWrapper = new ProcessMessageDelegateWrapper(handler);
//         _client->RegisterProcessMessageDelegates(delegateWrapper);
//     }
// }

bool CefWebView::openDevTools()
{
    if (_isDevToolsOpened) return true;

    if (_browser && _browser.get()) {
        CefWindowInfo windowInfo;
        windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
        windowInfo.SetAsWindowless(nullptr);
        CefBrowserSettings settings;
        _browser->GetHost()->ShowDevTools(windowInfo, _client, settings, CefPoint());
        _isDevToolsOpened = true;
    }
    return _isDevToolsOpened;
}

void CefWebView::closeDevTools()
{
    if (!_isDevToolsOpened)
        return;

    if (_browser && _browser.get()) {
        _browser->GetHost()->CloseDevTools();
        _isDevToolsOpened = false;
    }
}

void CefWebView::evaluateJavaScript(const std::string& script)
{
    if (script.empty()) {
        return;
    }

    if (_browser && _browser.get()) {
        CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
        if (frame) {
            frame->ExecuteJavaScript(CefString(script), frame->GetURL(), 0);
        }
    }
}

void CefWebView::setDeviceScaleFactor(float deviceScaleFactor)
{
    //if (!CefCurrentlyOn(TID_UI)) {
    //    // Execute this method on the UI thread.
    //    CefPostTask(TID_UI, base::BindOnce(&CefWebView::setDeviceScaleFactor, this, deviceScaleFactor));
    //    return;
    //}

    if (_deviceScaleFactor == deviceScaleFactor) {
        return;
    }

    _deviceScaleFactor = deviceScaleFactor;
    if (_browser) {
        _browser->GetHost()->NotifyScreenInfoChanged();
        _browser->GetHost()->WasResized();
    }
}

CefRefPtr<CefBrowser> CefWebView::getBrowser() const
{
    return _browser;
}

#pragma region RenderHandler
bool CefWebView::getRootScreenRect(CefRect& rect) {
    REQUIRE_UI_THREAD();
    if (!::IsWindow(_hwnd)) {
        return false;
    }
    HWND rootWindow = GetAncestor(_hwnd, GA_ROOT);
    rect = util::getWindowRect(rootWindow, _deviceScaleFactor);
    return true;
}

bool CefWebView::getScreenPoint(int viewX, int viewY, int& screenX, int& screenY) {
  REQUIRE_UI_THREAD();
  if (!::IsWindow(_hwnd)) {
    return false;
  }
  // Convert from view DIP coordinates to screen device (pixel) coordinates.
  POINT screenPt = { util::logicalToDevice(viewX, _deviceScaleFactor),
                     util::logicalToDevice(viewY, _deviceScaleFactor)};
  ClientToScreen(_hwnd, &screenPt);
  screenX = screenPt.x;
  screenY = screenPt.y;
  return true;
}

bool CefWebView::getScreenInfo(CefScreenInfo& screenInfo) {
    REQUIRE_UI_THREAD();
    if (!::IsWindow(_hwnd)) {
        return false;
    }

    CefRect viewRect;
    getViewRect(viewRect);

    screenInfo.device_scale_factor = _deviceScaleFactor;

    // The screen info rectangles are used by the renderer to create and position
    // popups. Keep popups inside the view rectangle.
    screenInfo.rect = viewRect;
    screenInfo.available_rect = viewRect;
    return true;
}

bool CefWebView::getViewRect(CefRect& rect){
    REQUIRE_UI_THREAD();

    rect.x = rect.y = 0;

    rect.width = util::deviceToLogical(_clientRect.right - _clientRect.left, _deviceScaleFactor);
    if (rect.width == 0) {
        rect.width = 1;
    }

    rect.height = util::deviceToLogical(_clientRect.bottom - _clientRect.top, _deviceScaleFactor);
    if (rect.height == 0) {
        rect.height = 1;
    }

    return true;
}

void CefWebView::onPaint(CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height)
{
    // Update frame data from CPU memory buffer
    _osrRenderer->updateFrameData(type, dirtyRects, buffer, width, height);

    // Trigger rendering
    _osrRenderer->render();
}

void CefWebView::onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                    const CefRenderHandler::RectList& dirtyRects,
                                    const CefAcceleratedPaintInfo& info)
{
    // Update D3D11 renderer using the shared texture handle from CEF
    _osrRenderer->updateSharedTexture(type, info);

    // Trigger rendering
    _osrRenderer->render();
}

// void onPopupShow(bool show);

// void onPopupSize(const CefRect &rect);

bool CefWebView::startDragging(CefRefPtr<CefDragData> dragData, CefRenderHandler::DragOperationsMask allowedOps, int x, int y)
{
    if (!_settings.offScreenRenderingEnabled) return false;
    if (!_dropTarget) return false;

    _currentDragOp = DRAG_OPERATION_NONE;
    CefBrowserHost::DragOperationsMask result = _dropTarget->StartDragging(_browser, dragData, allowedOps, x, y);
    _currentDragOp = DRAG_OPERATION_NONE;
    POINT pt = {};
    GetCursorPos(&pt);
    ::ScreenToClient(_hwnd, &pt);

    _browser->GetHost()->DragSourceEndedAt(
        util::deviceToLogical(pt.x, _deviceScaleFactor),
        util::deviceToLogical(pt.y, _deviceScaleFactor), result);
    _browser->GetHost()->DragSourceSystemDragEnded();
    return true;
}

void CefWebView::updateDragCursor(CefRenderHandler::DragOperation operation)
{
    _currentDragOp = operation;
}

void CefWebView::onImeCompositionRangeChanged(const CefRange& selectionRange, const CefRenderHandler::RectList& characterBounds)
{
    if (!_settings.offScreenRenderingEnabled) return;
    if (!_imeHandler) return;

    // Convert from view coordinates to device coordinates.
    CefRenderHandler::RectList deviceBounds;
    CefRenderHandler::RectList::const_iterator it = characterBounds.begin();
    for (; it != characterBounds.end(); ++it) {
        deviceBounds.push_back(util::logicalToDevice(*it, _deviceScaleFactor));
    }

    _imeHandler->changeCompositionRange(selectionRange, deviceBounds);
}

#pragma endregion // RenderHandler

 void CefWebView::onTitleChange(int browserId, const std::string& title)
 {
 }

 void CefWebView::onUrlChange(int browserId, const std::string& oldUrl, const std::string& url)
 {
     _settings.url = url;
 }

#pragma region LoadHandler
void CefWebView::onLoadingStateChange(int browserId, bool isLoading, bool canGoBack, bool canGoForward)
{
}

void CefWebView::onLoadStart(const std::string& url)
{
}

void CefWebView::onLoadEnd(const std::string& url)
{
}

void CefWebView::onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl)
{
}
#pragma endregion // LoadHandler

#pragma region LifeSpanHandler
void CefWebView::onAfterCreated(int browserId)
{
    if (_client) {
        _browser = _client->GetBrowser();
    }

    for (auto& task : _taskListAfterCreated) {
        if (task) {
            task();
        }
    }
    _taskListAfterCreated.clear();
}

 void CefWebView::onBeforeClose(int browserId)
 {
     _browser = nullptr;
 }
#pragma endregion // LifeSpanHandler

void CefWebView::onProcessMessageReceived(int browserId, const std::string& messageName, const std::string& jsonArgs)
{
    // Process message handling (extensible)
}

#pragma region windowProchandler
void CefWebView::onMouseEvent(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!_settings.offScreenRenderingEnabled || util::isMouseEventFromTouch(message)) return;

    CefRefPtr<CefBrowserHost> browserHost;
    if (_browser) {
        browserHost = _browser->GetHost();
    }

    LONG currentTime = 0;
    bool cancelPreviousClick = false;

    if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
        message == WM_MBUTTONDOWN || message == WM_MOUSEMOVE ||
        message == WM_MOUSELEAVE)
    {
        currentTime = GetMessageTime();
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        cancelPreviousClick =
            (abs(_lastClickX - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2)) ||
            (abs(_lastClickY - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2)) ||
            ((currentTime - _lastClickTime) > GetDoubleClickTime());
        if (cancelPreviousClick && (message == WM_MOUSEMOVE || message == WM_MOUSELEAVE)) {
            _lastClickCount = 1;
            _lastClickX = 0;
            _lastClickY = 0;
            _lastClickTime = 0;
        }
    }

    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        ::SetCapture(_hwnd);
        ::SetFocus(_hwnd);
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (wParam & MK_SHIFT) {
            // Start rotation effect.
            _lastMousePos.x = _currentMousePos.x = x;
            _lastMousePos.y = _currentMousePos.y = y;
            _mouseRotation = true;
        }
        else {
            CefBrowserHost::MouseButtonType btnType = (message == WM_LBUTTONDOWN ? MBT_LEFT : (message == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
            if (!cancelPreviousClick && (btnType == _lastClickButton)) {
                ++_lastClickCount;
            } else {
                _lastClickCount = 1;
                _lastClickX = x;
                _lastClickY = y;
            }
            _lastClickTime = currentTime;
            _lastClickButton = btnType;

            if (browserHost) {
                CefMouseEvent mouseEvent;
                mouseEvent.x = x;
                mouseEvent.y = y;
                util::deviceToLogical(mouseEvent, _deviceScaleFactor);
                mouseEvent.modifiers = util::getCefMouseModifiers(wParam);
                browserHost->SendMouseClickEvent(mouseEvent, btnType, false,
                                                 _lastClickCount);
            }
        }
    }
    break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        if (::GetCapture() == _hwnd) {
            ::ReleaseCapture();
        }
        if (_mouseRotation) {
            // End rotation effect.
            _mouseRotation = false;
            // _osrRenderer->setSpin(0, 0);
        }
        else {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            CefBrowserHost::MouseButtonType btnType = (message == WM_LBUTTONUP ? MBT_LEFT : (message == WM_RBUTTONUP ? MBT_RIGHT : MBT_MIDDLE));
            if (browserHost) {
                CefMouseEvent mouseEvent;
                mouseEvent.x = x;
                mouseEvent.y = y;
                util::deviceToLogical(mouseEvent, _deviceScaleFactor);
                mouseEvent.modifiers = util::getCefMouseModifiers(wParam);
                browserHost->SendMouseClickEvent(mouseEvent, btnType, true,
                                                 _lastClickCount);
            }
        }
        break;

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (_mouseRotation) {
            // Apply rotation effect.
            _currentMousePos.x = x;
            _currentMousePos.y = y;
            // _osrRenderer->IncrementSpin(
            //     _currentMousePos.x - _lastMousePos.x,
            //     _currentMousePos.y - _lastMousePos.y);
            _lastMousePos.x = _currentMousePos.x;
            _lastMousePos.y = _currentMousePos.y;
        }
        else {
            if (!_mouseTracking) {
                // Start tracking mouse leave. Required for the WM_MOUSELEAVE event to
                // be generated.
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = _hwnd;
                ::TrackMouseEvent(&tme);
                _mouseTracking = true;
            }

            if (browserHost) {
                CefMouseEvent mouseEvent;
                mouseEvent.x = x;
                mouseEvent.y = y;
                util::deviceToLogical(mouseEvent, _deviceScaleFactor);
                mouseEvent.modifiers = util::getCefMouseModifiers(wParam);
                browserHost->SendMouseMoveEvent(mouseEvent, false);
            }
        }
        break;
    }

    case WM_MOUSELEAVE: {
        if (_mouseTracking) {
            // Stop tracking mouse leave.
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE & TME_CANCEL;
            tme.hwndTrack = _hwnd;
            ::TrackMouseEvent(&tme);
            _mouseTracking = false;
        }

        if (browserHost) {
            // Determine the cursor position in screen coordinates.
            POINT p;
            ::GetCursorPos(&p);
            ::ScreenToClient(_hwnd, &p);

            CefMouseEvent mouseEvent;
            mouseEvent.x = p.x;
            mouseEvent.y = p.y;
            util::deviceToLogical(mouseEvent, _deviceScaleFactor);
            mouseEvent.modifiers = util::getCefMouseModifiers(wParam);
            browserHost->SendMouseMoveEvent(mouseEvent, true);
        }
    }
    break;

    case WM_MOUSEWHEEL:
        if (browserHost) {
            POINT screenPoint = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            HWND scrolledWnd = ::WindowFromPoint(screenPoint);
            if (scrolledWnd != _hwnd) {
                break;
            }

            ::ScreenToClient(_hwnd, &screenPoint);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int deltaX = util::isKeyDown(VK_SHIFT) ? delta : 0;
            int deltaY = !util::isKeyDown(VK_SHIFT) ? delta : 0;

            CefMouseEvent mouseEvent;
            mouseEvent.x = screenPoint.x;
            mouseEvent.y = screenPoint.y;
            util::deviceToLogical(mouseEvent, _deviceScaleFactor);
            mouseEvent.modifiers = util::getCefMouseModifiers(wParam);

            UINT sys_info_lines;
            if (SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &sys_info_lines, 0) && sys_info_lines == WHEEL_PAGESCROLL) {
                mouseEvent.modifiers |= EVENTFLAG_SCROLL_BY_PAGE;
                deltaX = 0;
                deltaY = (delta > 0) ? 1 : -1;
            }

            browserHost->SendMouseWheelEvent(mouseEvent, deltaX, deltaY);
        }
        break;
    }
}

void CefWebView::onSize() {
    if (!_settings.offScreenRenderingEnabled) return;

    // Keep client_rect_ up to date
    ::GetClientRect(_hwnd, &_clientRect);

    int width = _clientRect.right - _clientRect.left;
    int height = _clientRect.bottom - _clientRect.top;

    // Update D3D11 renderer size
    if (_osrRenderer && width > 0 && height > 0) {
        _osrRenderer->resize(width, height);
    }

    // Notify CEF browser about size change
    if (_browser) {
        _browser->GetHost()->WasResized();
    }
}

void CefWebView::onFocus(bool setFocus) {
    if (!_settings.offScreenRenderingEnabled) return;

    if (_browser) {
        _browser->GetHost()->SetFocus(setFocus);
    }
}

void CefWebView::onCaptureLost() {
    if (!_settings.offScreenRenderingEnabled) return;

    if (_mouseRotation) {
        return;
    }

    if (_browser) {
        _browser->GetHost()->SendCaptureLostEvent();
    }
}

void CefWebView::onKeyEvent(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!_settings.offScreenRenderingEnabled) return;

    if (!_browser) return;

    CefKeyEvent event;
    event.windows_key_code = wParam;
    event.native_key_code = lParam;
    event.is_system_key = message == WM_SYSCHAR || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP;

    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        event.type = KEYEVENT_RAWKEYDOWN;
    }
    else if (message == WM_KEYUP || message == WM_SYSKEYUP) {
        event.type = KEYEVENT_KEYUP;
    }
    else {
        event.type = KEYEVENT_CHAR;
    }
    event.modifiers = util::getCefKeyboardModifiers(wParam, lParam);

    // mimic alt-gr check behaviour from
    // src/ui/events/win/events_win_utils.cc: GetModifiersFromKeyState
    if ((event.type == KEYEVENT_CHAR) && util::isKeyDown(VK_RMENU)) {
        // reverse AltGr detection taken from PlatformKeyMap::UsesAltGraph
        // instead of checking all combination for ctrl-alt, just check current char
        HKL current_layout = ::GetKeyboardLayout(0);

        // https://docs.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-vkkeyscanexw
        // ... high-order byte contains the shift state,
        // which can be a combination of the following flag bits.
        // 1 Either SHIFT key is pressed.
        // 2 Either CTRL key is pressed.
        // 4 Either ALT key is pressed.
        SHORT scan_res = ::VkKeyScanExW(wParam, current_layout);
        constexpr auto ctrlAlt = (2 | 4);
        if (((scan_res >> 8) & ctrlAlt) == ctrlAlt)  {
            // ctrl-alt pressed
            event.modifiers &= ~(EVENTFLAG_CONTROL_DOWN | EVENTFLAG_ALT_DOWN);
            event.modifiers |= EVENTFLAG_ALTGR_DOWN;
        }
    }

    _browser->GetHost()->SendKeyEvent(event);
}

void CefWebView::onPaint() {
    if (!_settings.offScreenRenderingEnabled) return;
    // Paint nothing here. Invalidate will cause OnPaint to be called for the

    _osrRenderer->render();

    if (_browser) {
        _browser->GetHost()->Invalidate(PET_VIEW);
    }
}

bool CefWebView::onTouchEvent(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!_settings.offScreenRenderingEnabled) return false;

    // Handle touch events on Windows.
    int num_points = LOWORD(wParam);
    // Chromium only supports upto 16 touch points.
    if (num_points < 0 || num_points > 16) {
        return false;
    }
    std::unique_ptr<TOUCHINPUT[]> input(new TOUCHINPUT[num_points]);
    if (GetTouchInputInfo(reinterpret_cast<HTOUCHINPUT>(lParam), num_points, input.get(), sizeof(TOUCHINPUT))) {
        CefTouchEvent touch_event;
        for (int i = 0; i < num_points; ++i) {
            POINT point;
            point.x = TOUCH_COORD_TO_PIXEL(input[i].x);
            point.y = TOUCH_COORD_TO_PIXEL(input[i].y);

            if (!util::isWindows8OrNewer()) {
                // Windows 7 sends touch events for touches in the non-client area,
                // whereas Windows 8 does not. In order to unify the behaviour, always
                // ignore touch events in the non-client area.
                LPARAM l_param_ht = MAKELPARAM(point.x, point.y);
                LRESULT hittest = SendMessage(_hwnd, WM_NCHITTEST, 0, l_param_ht);
                if (hittest != HTCLIENT) {
                    return false;
                }
            }

            ::ScreenToClient(_hwnd, &point);
            touch_event.x = util::deviceToLogical(point.x, _deviceScaleFactor);
            touch_event.y = util::deviceToLogical(point.y, _deviceScaleFactor);

            // Touch point identifier stays consistent in a touch contact sequence
            touch_event.id = input[i].dwID;
            if (input[i].dwFlags & TOUCHEVENTF_DOWN) {
                touch_event.type = CEF_TET_PRESSED;
            }
            else if (input[i].dwFlags & TOUCHEVENTF_MOVE) {
                touch_event.type = CEF_TET_MOVED;
            }
            else if (input[i].dwFlags & TOUCHEVENTF_UP) {
                touch_event.type = CEF_TET_RELEASED;
            }

            touch_event.radius_x = 0;
            touch_event.radius_y = 0;
            touch_event.rotation_angle = 0;
            touch_event.pressure = 0;
            touch_event.modifiers = 0;

            // Notify the browser of touch event
            if (_browser) {
                _browser->GetHost()->SendTouchEvent(touch_event);
            }
        }
        ::CloseTouchInputHandle(reinterpret_cast<HTOUCHINPUT>(lParam));
        return true;
    }

    return false;
}

void CefWebView::onIMESetContext(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!_settings.offScreenRenderingEnabled) return;
    // We handle the IME Composition Window ourselves (but let the IME Candidates
    // Window be handled by IME through DefWindowProc()), so clear the
    // ISC_SHOWUICOMPOSITIONWINDOW flag:
    lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
    ::DefWindowProc(_hwnd, message, wParam, lParam);

    // Create Caret Window if required
    if (_imeHandler) {
        _imeHandler->createImeWindow();
        _imeHandler->moveImeWindow();
    }
}

void CefWebView::onIMEStartComposition() {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_imeHandler) {
        _imeHandler->createImeWindow();
        _imeHandler->moveImeWindow();
        _imeHandler->resetComposition();
    }
}

void CefWebView::onIMEComposition(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!_settings.offScreenRenderingEnabled) return;
    if (!_browser || !_imeHandler) return;

    CefString cTextStr;
    if (_imeHandler->getResult(lParam, cTextStr)) {
        // Send the text to the browser. The |replacement_range| and
        // |relative_cursor_pos| params are not used on Windows, so provide
        // default invalid values.
        _browser->GetHost()->ImeCommitText(cTextStr, CefRange::InvalidRange(), 0);
        _imeHandler->resetComposition();
        // Continue reading the composition string - Japanese IMEs send both
        // GCS_RESULTSTR and GCS_COMPSTR.
    }

    std::vector<CefCompositionUnderline> underlines;
    int compositionStart = 0;
    if (_imeHandler->getComposition(lParam, cTextStr, underlines, compositionStart)) {
        // Send the composition string to the browser. The |replacement_range|
        // param is not used on Windows, so provide a default invalid value.
        _browser->GetHost()->ImeSetComposition(
            cTextStr, underlines, CefRange::InvalidRange(),
            CefRange(compositionStart, static_cast<int>(compositionStart + cTextStr.length())));

        // Update the Candidate Window position. The cursor is at the end so
        // subtract 1. This is safe because IMM32 does not support non-zero-width
        // in a composition. Also,  negative values are safely ignored in
        // MoveImeWindow
        _imeHandler->updateCaretPosition(compositionStart - 1);
    }
    else {
        onIMECancelCompositionEvent();
    }
}

void CefWebView::onIMECancelCompositionEvent() {
    if (!_settings.offScreenRenderingEnabled) return;
    if (!_browser || !_imeHandler) return;

    _browser->GetHost()->ImeCancelComposition();
    _imeHandler->resetComposition();
    _imeHandler->destroyImeWindow();
}

#pragma endregion // windowProchandler

#pragma region dragEvents
CefBrowserHost::DragOperationsMask CefWebView::onDragEnter(CefRefPtr<CefDragData> dragData, CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) {
    if (_browser) {
        util::deviceToLogical(ev, _deviceScaleFactor);
        _browser->GetHost()->DragTargetDragEnter(dragData, ev, effect);
        _browser->GetHost()->DragTargetDragOver(ev, effect);
    }
    return _currentDragOp;
}

CefBrowserHost::DragOperationsMask CefWebView::onDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) {
    if (_browser) {
        util::deviceToLogical(ev, _deviceScaleFactor);
        _browser->GetHost()->DragTargetDragOver(ev, effect);
    }
    return _currentDragOp;
}

void CefWebView::onDragLeave() {
    if (_browser) {
        _browser->GetHost()->DragTargetDragLeave();
    }
}

CefBrowserHost::DragOperationsMask CefWebView::onDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect) {
    if (_browser) {
        util::deviceToLogical(ev, _deviceScaleFactor);
        _browser->GetHost()->DragTargetDragOver(ev, effect);
        _browser->GetHost()->DragTargetDrop(ev);
    }
    return _currentDragOp;
}
#pragma endregion // dragEvents

}