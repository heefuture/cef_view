#include "MainWindow.h"
#include <view/CefWebView.h>
#include <view/CefWebViewSetting.h>
#include <utils/WinUtil.h>

#include <optional>
#include <shellscalingapi.h>

using namespace cefview;

#define MAX_URL_LENGTH 255

#define BUTTON_WIDTH 72
#define BUTTON_HEIGHT 30
#define URLBAR_HEIGHT 24
#define ID_BTN_SWITCH1 101
#define ID_BTN_SWITCH2 102
#define ID_BTN_SWITCH3 103

// 资源ID定义
#define IDS_APP_TITLE      103
#define IDR_MAINFRAME      128
#define IDI_SMALL          108
#define IDM_ABOUT          104
#define IDM_EXIT           105
#define IDD_ABOUTBOX       103

// Message handler for the About box.
INT_PTR CALLBACK AboutWndProc(HWND hDlg,
                              UINT message,
                              WPARAM wParam,
                              LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        } break;
    }
    return FALSE;
}

int GetButtonWidth(HWND hwnd)
{
    return static_cast<int>(BUTTON_WIDTH * WinUtil::GetWindowScaleFactor(hwnd));
}

int GetURLBarHeight(HWND hwnd)
{
    return static_cast<int>(URLBAR_HEIGHT * WinUtil::GetWindowScaleFactor(hwnd));
}


MainWindow::MainWindow()
{
    // Create a HRGN representing the draggable window area.
    _draggableRegion = ::CreateRectRgn(0, 0, 0, 0);
}

MainWindow::~MainWindow()
{
    ::DeleteObject(_draggableRegion);

    // The window and browser should already have been destroyed.
    assert(_windowDestroyed);
}

void MainWindow::init(std::unique_ptr<Config> config)
{
    if (_initialized) return;

    _alwaysOnTop = config->alwaysOnTop;
    _initialBounds = config->bounds;
    _initialShowMode = config->showMode;

    createRootWindow(config->initiallyHidden);

    createCefViews();
    updateLayout();

    _initialized = true;
}

void MainWindow::show(ShowMode mode)
{
    if (!_hwnd) return;

    int nCmdShow = SW_SHOWNORMAL;
    switch (mode)
    {
    case ShowMode::MINIMIZED:
        nCmdShow = SW_SHOWMINIMIZED;
        break;
    case ShowMode::MAXIMIZED:
        nCmdShow = SW_SHOWMAXIMIZED;
        break;
    case ShowMode::NO_ACTIVATE:
        nCmdShow = SW_SHOWNOACTIVATE;
        break;
    default:
        break;
    }

    ShowWindow(_hwnd, nCmdShow);
    if (mode != ShowMode::MINIMIZED) {
        UpdateWindow(_hwnd);
    }
}

void MainWindow::hide()
{
    if (_hwnd){
        ShowWindow(_hwnd, SW_HIDE);
    }
}

void MainWindow::setBounds(int x, int y, size_t width, size_t height)
{
    if (_hwnd){
        SetWindowPos(_hwnd, nullptr, x, y, static_cast<int>(width),
                     static_cast<int>(height), SWP_NOZORDER);
    }
}

void MainWindow::close(bool force)
{
    if (_hwnd) {
        if (force) {
            DestroyWindow(_hwnd);
        } else {
            PostMessage(_hwnd, WM_CLOSE, 0, 0);
        }
    }
}

void MainWindow::setDeviceScaleFactor(float deviceScaleFactor)
{
     for (const auto& cefView : _bottomViews) {
         cefView->setDeviceScaleFactor(deviceScaleFactor);
     }
     _topView->setDeviceScaleFactor(deviceScaleFactor);
    _deviceScaleFactor = deviceScaleFactor;
}

float MainWindow::getDeviceScaleFactor() const
{
    return _deviceScaleFactor;
}

// CefRefPtr<CefBrowser> MainWindow::getBrowser() const
// {
//     if (_cefViewList.empty()) {
//         return nullptr;
//     }
//     return _cefViewList.front()->GetBrowser();
// }

HWND MainWindow::getWindowHandle() const
{
    return _hwnd;
}

void MainWindow::createRootWindow(bool initiallyHidden)
{
    if (_hwnd) return;

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // 窗口标题和类名
    const std::wstring windowTitle = L"CEF View Demo Application";
    const std::wstring windowClass = L"CefViewMainWindow";

    const HBRUSH backgroundBrush = nullptr;

    // Register the window class.
    RegisterRootClass(hInstance, windowClass, backgroundBrush);

    DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    DWORD dwExStyle = _alwaysOnTop ? WS_EX_TOPMOST : 0;
    if (_initialShowMode == ShowMode::NO_ACTIVATE) {
        // Don't activate the browser window on creation.
        dwExStyle |= WS_EX_NOACTIVATE;
    }

    if (_initialShowMode == ShowMode::MAXIMIZED) {
        dwStyle |= WS_MAXIMIZE;
    }
    else if (_initialShowMode == ShowMode::MINIMIZED) {
        dwStyle |= WS_MINIMIZE;
    }

    int x, y, width, height;
    if (_initialBounds.right - _initialBounds.left == 0 ||
        _initialBounds.bottom - _initialBounds.top == 0) {
        // Use the default window position/size.
        x = y = width = height = CW_USEDEFAULT;
    }
    else {
        x = _initialBounds.left;
        y = _initialBounds.top;
        width = _initialBounds.right - _initialBounds.left;
        height = _initialBounds.bottom - _initialBounds.top;
    }

    // Create the main window initially hidden.
    _hwnd = CreateWindowEx(dwExStyle, windowClass.c_str(), windowTitle.c_str(), dwStyle,
                           x, y, width, height, nullptr, nullptr, hInstance, this);

    if (!_calledEnableNonClientDpiScaling && WinUtil::IsProcessPerMonitorDpiAware()) {
        // This call gets Windows to scale the non-client area when WM_DPICHANGED
        // is fired on Windows versions < 10.0.14393.0.
        // Derived signature; not available in headers.
        typedef LRESULT(WINAPI * EnableChildWindowDpiMessagePtr)(HWND, BOOL);
        static EnableChildWindowDpiMessagePtr funcPtr =
            reinterpret_cast<EnableChildWindowDpiMessagePtr>(GetProcAddress(
                GetModuleHandle(L"user32.dll"), "EnableChildWindowDpiMessage"));
        if (funcPtr) {
            funcPtr(_hwnd, TRUE);
        }
    }

    if (!initiallyHidden) {
        // Show this window.
        show(_initialShowMode);
    }
}

// static
void MainWindow::RegisterRootClass(HINSTANCE hInstance,
                                   const std::wstring &windowClass,
                                   HBRUSH backgroundBrush)
{
    // Only register the class one time.
    static bool classRegistered = false;
    if (classRegistered)
    {
        return;
    }
    classRegistered = true;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = RootWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;  // LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = backgroundBrush;
    wcex.lpszMenuName = nullptr;  // MAKEINTRESOURCE(IDR_MAINFRAME);
    wcex.lpszClassName = windowClass.c_str();
    wcex.hIconSm = nullptr;  // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassEx(&wcex);
}


// static
LRESULT CALLBACK MainWindow::RootWndProc(HWND hWnd,
                                         UINT message,
                                         WPARAM wParam,
                                         LPARAM lParam)
{
    MainWindow *self = nullptr;
    if (message != WM_NCCREATE)
    {
        self = WinUtil::GetUserDataPtr<MainWindow*>(hWnd);
        if (!self) {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }

    // Callback for the main window
    switch (message)
    {
    case WM_COMMAND:
        if (self->onCommand(LOWORD(wParam))) {
            return 0;
        } break;
    case WM_PAINT:
        self->onPaint(); break;
    case WM_ACTIVATE:
        self->onActivate(LOWORD(wParam) != WA_INACTIVE); break;
    case WM_SETFOCUS:
        self->onFocus(); break;
    case WM_ENABLE:
        if (wParam == TRUE) {
            // Give focus to the browser after EnableWindow enables this window
            // (e.g. after a modal dialog is dismissed).
            self->onFocus();
            return 0;
        } break;

    case WM_SIZE:
        self->onSize(wParam == SIZE_MINIMIZED); break;

    case WM_MOVING:
    case WM_MOVE:
        self->onMove();
        return 0;

    case WM_DPICHANGED:
        self->onDpiChanged(wParam, lParam); break;

    case WM_ERASEBKGND:
        if (self->onEraseBkgnd()) {
            break;
        }
        // Don't erase the background.
        return 0;

    // case WM_ENTERMENULOOP:
    //     if (!wParam)
    //     {
    //         // Entering the menu loop for the application menu.
    //         CefSetOSModalLoop(true);
    //     }
    //     break;

    // case WM_EXITMENULOOP:
    //     if (!wParam)
    //     {
    //         // Exiting the menu loop for the application menu.
    //         CefSetOSModalLoop(false);
    //     }
    //     break;

    case WM_CLOSE:
        if (self->onClose()) {
            return 0; // Cancel the close.
        } break;
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINTS points = MAKEPOINTS(lParam);
            POINT point = {points.x, points.y};
            ::ScreenToClient(hWnd, &point);
            if (::PtInRegion(self->_draggableRegion, point.x, point.y))
            {
                // If cursor is inside a draggable region return HTCAPTION to allow
                // dragging.
                return HTCAPTION;
            }
        }
        return hit;
    }
    case WM_NCCREATE: {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
        self = reinterpret_cast<MainWindow *>(cs->lpCreateParams);
        assert(self);
        // Associate |self| with the main window.
        WinUtil::SetUserDataPtr(hWnd, self);
        self->_hwnd = hWnd;

        self->onNCCreate(cs);
    } break;

    case WM_CREATE:
        self->onCreate(reinterpret_cast<CREATESTRUCT *>(lParam)); break;

    case WM_NCDESTROY:
        // Clear the reference to |self|.
        WinUtil::SetUserDataPtr(hWnd, nullptr);
        self->_hwnd = nullptr;
        self->onDestroyed();
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void MainWindow::onPaint()
{
    // PAINTSTRUCT ps;
    // BeginPaint(_hwnd, &ps);
    // EndPaint(_hwnd, &ps);
}

void MainWindow::onFocus()
{
    //// T115: 给顶部视图焦点
    //if (_topView && ::IsWindowEnabled(_hwnd)) {
    //    ::SetFocus(_topView->getWindowHandle());
    //}
}

void MainWindow::onActivate(bool active)
{
}

void MainWindow::onSize(bool minimized)
{
    if (minimized) {
        // 最小化时隐藏所有视图
        if (_topView) {
            _topView->setVisible(false);
        }
        for (auto& view : _bottomViews) {
            view->setVisible(false);
        }
        return;
    }

    // T113: 窗口大小变化时更新布局
    updateLayout();

    // 恢复可见性
    if (_topView) {
        _topView->setVisible(true);
    }
    if (_activeBottomViewIndex >= 0 && _activeBottomViewIndex < static_cast<int>(_bottomViews.size())) {
        _bottomViews[_activeBottomViewIndex]->setVisible(true);
    }
}

void MainWindow::onMove()
{
    //// 通知所有浏览器窗口移动（用于弹出窗口正确定位）
    //if (_topView) {
    //    auto browser = _topView->getBrowser();
    //    if (browser) {
    //        browser->GetHost()->NotifyMoveOrResizeStarted();
    //    }
    //}

    //for (auto& view : _bottomViews) {
    //    auto browser = view->getBrowser();
    //    if (browser) {
    //        browser->GetHost()->NotifyMoveOrResizeStarted();
    //    }
    //}
}

// DPI value for 1x scale factor.
#define DPI_1X 96.0f

void MainWindow::onDpiChanged(WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(wParam) != HIWORD(wParam)) {
        // NOTIMPLEMENTED() << "Received non-square scaling factors";
        return;
    }

    // T118-T119: 更新设备缩放因子并通知所有CefWebView
    const float displayScaleFactor = static_cast<float>(LOWORD(wParam)) / DPI_1X;
    setDeviceScaleFactor(displayScaleFactor);

    // T119: 调整窗口大小和位置
    const RECT* rect = reinterpret_cast<RECT*>(lParam);
    setBounds(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
}

bool MainWindow::onEraseBkgnd()
{
    // Erase the background when the browser does not exist.
    // return (browser_window_ == nullptr);
    return false; // Do not erase the background.
}

bool MainWindow::onCommand(UINT id)
{
    // T115: 处理按钮点击消息切换底部视图
    switch (id) {
    case ID_BTN_SWITCH1:
        setActiveBottomView(0);
        return true;
    case ID_BTN_SWITCH2:
        setActiveBottomView(1);
        return true;
    case ID_BTN_SWITCH3:
        setActiveBottomView(2);
        return true;
    case IDM_ABOUT:
        onAbout();
        return true;
    case IDM_EXIT:
        close(false);
        return true;
    }

    return false;
}


void MainWindow::onAbout()
{
    // Show the about box.
    MessageBox(_hwnd, L"CEF View Demo Application Version 1.0", L"About", MB_OK);
}

void MainWindow::onNCCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (WinUtil::IsProcessPerMonitorDpiAware())
    {
        // This call gets Windows to scale the non-client area when WM_DPICHANGED
        // is fired on Windows versions >= 10.0.14393.0.
        typedef BOOL(WINAPI * EnableNonClientDpiScalingPtr)(HWND);
        static EnableNonClientDpiScalingPtr funcPtr =
            reinterpret_cast<EnableNonClientDpiScalingPtr>(GetProcAddress(
                GetModuleHandle(L"user32.dll"), "EnableNonClientDpiScaling"));
        _calledEnableNonClientDpiScaling = !!(funcPtr && funcPtr(_hwnd));
    }
}

void MainWindow::onCreate(LPCREATESTRUCT lpCreateStruct)
{
    _windowCreated = true;
}

// ============================================================================
// T105: 实现createCefViews()创建顶部和底部视图
// ============================================================================

void MainWindow::createCefViews()
{
    createTopView();
    createBottomViews();
}

// ============================================================================
// T106-T108: 实现createTopView()创建窗口模式CefWebView
// ============================================================================

void MainWindow::createTopView()
{
    if (!_hwnd) return;

    // Calculate correct initial size based on current window size
    RECT clientRect;
    GetClientRect(_hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    int buttonAreaHeight = BUTTON_HEIGHT + 20;
    int availableHeight = clientHeight - buttonAreaHeight;
    int topHeight = availableHeight / 2;

    CefWebViewSetting settings;
    settings.offScreenRenderingEnabled = true;
    settings.x = 0;
    settings.y = 0;
    settings.width = clientWidth;
    settings.height = topHeight;
    settings.url = "https://www.baidu.com";

    _topView = std::make_shared<CefWebView>(settings, _hwnd);
}

void MainWindow::createBottomViews()
{
    if (!_hwnd) return;

    // Calculate correct initial size based on current window size
    RECT clientRect;
    GetClientRect(_hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    int buttonAreaHeight = BUTTON_HEIGHT + 20;
    int availableHeight = clientHeight - buttonAreaHeight;
    int topHeight = availableHeight / 2;
    int bottomHeight = availableHeight - topHeight;

    // T080: Create 3 bottom views
    const char* urls[] = {
        "https://www.bing.com",
        "https://www.google.com",
        "https://github.com"
    };

    for (int i = 0; i < 3; ++i) {
        CefWebViewSetting settings;
        settings.x = 0;
        settings.y = topHeight;
        settings.width = clientWidth;
        settings.height = bottomHeight;
        settings.url = urls[i];

        auto view = std::make_shared<CefWebView>(settings, _hwnd);

        if (i > 0) {
            view->setVisible(false);
        }

        _bottomViews.push_back(view);
    }

    _activeBottomViewIndex = 0;

    // T085-T086: 创建切换按钮
    int buttonY = clientRect.bottom - BUTTON_HEIGHT - 10;
    int buttonSpacing = 10;

    _btnSwitch1 = CreateWindow(
        L"BUTTON", L"View 1",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
        _hwnd, (HMENU)ID_BTN_SWITCH1, GetModuleHandle(nullptr), nullptr);

    _btnSwitch2 = CreateWindow(
        L"BUTTON", L"View 2",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10 + BUTTON_WIDTH + buttonSpacing, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
        _hwnd, (HMENU)ID_BTN_SWITCH2, GetModuleHandle(nullptr), nullptr);

    _btnSwitch3 = CreateWindow(
        L"BUTTON", L"View 3",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10 + (BUTTON_WIDTH + buttonSpacing) * 2, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
        _hwnd, (HMENU)ID_BTN_SWITCH3, GetModuleHandle(nullptr), nullptr);
}

void MainWindow::setActiveBottomView(int index)
{
    if (index < 0 || index >= static_cast<int>(_bottomViews.size())) {
        return;
    }

    if (index == _activeBottomViewIndex) {
        return;  // 已经是激活状态
    }

    // 隐藏当前激活的视图
    if (_activeBottomViewIndex >= 0 && _activeBottomViewIndex < static_cast<int>(_bottomViews.size())) {
        _bottomViews[_activeBottomViewIndex]->setVisible(false);
    }

    // T083: 显示新选中的视图
    _activeBottomViewIndex = index;
    _bottomViews[_activeBottomViewIndex]->setVisible(true);

    // T084: 触发重绘
    InvalidateRect(_hwnd, nullptr, TRUE);
    UpdateWindow(_hwnd);
}

// ============================================================================
// T109-T113: 实现updateLayout()布局管理
// ============================================================================

void MainWindow::updateLayout()
{
    if (!_hwnd) return;

    RECT clientRect;
    GetClientRect(_hwnd, &clientRect);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // T110: 计算顶部/底部区域矩形（各占50%高度，底部留出按钮空间）
    int buttonAreaHeight = BUTTON_HEIGHT + 20;  // 按钮高度 + 边距
    int availableHeight = clientHeight - buttonAreaHeight;
    int topHeight = availableHeight / 2;
    int bottomHeight = availableHeight - topHeight;

    // T111: 更新顶部视图布局
    if (_topView) {
        _topView->setBounds(0, 0, clientWidth, topHeight);
    }

    // T112: 更新所有底部视图布局（叠加在同一区域）
    for (auto& view : _bottomViews) {
        view->setBounds(0, topHeight, clientWidth, bottomHeight);
    }

    // 更新按钮位置
    int buttonY = clientHeight - BUTTON_HEIGHT - 10;
    int buttonSpacing = 10;

    if (_btnSwitch1) {
        SetWindowPos(_btnSwitch1, nullptr,
                     10, buttonY, BUTTON_WIDTH, BUTTON_HEIGHT,
                     SWP_NOZORDER);
    }
    if (_btnSwitch2) {
        SetWindowPos(_btnSwitch2, nullptr,
                     10 + BUTTON_WIDTH + buttonSpacing, buttonY,
                     BUTTON_WIDTH, BUTTON_HEIGHT,
                     SWP_NOZORDER);
    }
    if (_btnSwitch3) {
        SetWindowPos(_btnSwitch3, nullptr,
                     10 + (BUTTON_WIDTH + buttonSpacing) * 2, buttonY,
                     BUTTON_WIDTH, BUTTON_HEIGHT,
                     SWP_NOZORDER);
    }

    InvalidateRect(_hwnd, nullptr, TRUE);
    UpdateWindow(_hwnd);
}

bool MainWindow::onClose()
{
    // T125: 确保所有CefWebView正确销毁
    if (_topView) {
        _topView.reset();
    }

    for (auto& view : _bottomViews) {
        view.reset();
    }
    _bottomViews.clear();

    // Allow the close.
    return false;
}

void MainWindow::onDestroyed()
{
    _windowDestroyed = true;
}

