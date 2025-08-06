#include "MainWindow.h"

#include <utils/geometry_util.h>

#include <optional>
#include <shellscalingapi.h>

#define MAX_URL_LENGTH 255

#define BUTTON_WIDTH 72
#define URLBAR_HEIGHT 24

namespace
{

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

// Returns true if the process is per monitor DPI aware.
bool IsProcessPerMonitorDpiAware()
{
    enum class PerMonitorDpiAware {
        UNKNOWN = 0,
        PER_MONITOR_DPI_UNAWARE,
        PER_MONITOR_DPI_AWARE,
    };
    static PerMonitorDpiAware perMonitorDpiAware = PerMonitorDpiAware::UNKNOWN;
    if (perMonitorDpiAware == PerMonitorDpiAware::UNKNOWN) {
        perMonitorDpiAware = PerMonitorDpiAware::PER_MONITOR_DPI_UNAWARE;
        HMODULE shcoreDll = ::LoadLibrary(L"shcore.dll");
        if (shcoreDll) {
            typedef HRESULT(WINAPI * GetProcessDpiAwarenessPtr)(HANDLE, PROCESS_DPI_AWARENESS *);
            GetProcessDpiAwarenessPtr funcPtr =
                reinterpret_cast<GetProcessDpiAwarenessPtr>(
                    ::GetProcAddress(shcoreDll, "GetProcessDpiAwareness"));
            if (funcPtr) {
                PROCESS_DPI_AWARENESS awareness;
                if (SUCCEEDED(funcPtr(nullptr, &awareness)) &&
                    awareness == PROCESS_PER_MONITOR_DPI_AWARE) {
                    perMonitorDpiAware = PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
                }
            }
        }
    }
    return perMonitorDpiAware == PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
}

// DPI value for 1x scale factor.
#define DPI_1X 96.0f

float GetWindowScaleFactor(HWND hwnd)
{
    if (hwnd && IsProcessPerMonitorDpiAware()) {
        typedef UINT(WINAPI * GetDpiForWindowPtr)(HWND);
        static GetDpiForWindowPtr funcPtr = reinterpret_cast<GetDpiForWindowPtr>(
            GetProcAddress(GetModuleHandle(L"user32.dll"), "GetDpiForWindow"));
        if (funcPtr) {
            return static_cast<float>(funcPtr(hwnd)) / DPI_1X;
        }
    }

    return client::GetDeviceScaleFactor();
}

int GetButtonWidth(HWND hwnd)
{
    return client::LogicalToDevice(BUTTON_WIDTH, GetWindowScaleFactor(hwnd));
}

int GetURLBarHeight(HWND hwnd)
{
    return client::LogicalToDevice(URLBAR_HEIGHT, GetWindowScaleFactor(hwnd));
}

} // namespace


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

    showWindow(_hwnd, nCmdShow);
    if (mode != ShowMode::MINIMIZED) {
        updateWindow(_hwnd);
    }
}

void MainWindow::hide()
{
    if (_hwnd){
        showWindow(_hwnd, SW_HIDE);
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
    // for (const auto& cefView : _cefViewList) {
    //     cefView->SetDeviceScaleFactor(deviceScaleFactor);
    // }
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

    // Load strings from the resource file.
    // const std::wstring &windowTitle = GetResourceString(IDS_APP_TITLE);
    // const std::wstring &windowClass = GetResourceString(IDR_MAINFRAME);
    const std::wstring &windowTitle = GetResourceString(IDS_APP_TITLE);
    const std::wstring &windowClass = GetResourceString(IDR_MAINFRAME);

    // const HBRUSH backgroundBrush = CreateSolidBrush(RGB(255, 255, 255);
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
    _hwnd = CreateWindowEx(dwExStyle, window_class.c_str(), window_title.c_str(), dwStyle,
                           x, y, width, height, nullptr, nullptr, hInstance, this);

    if (!_calledEnableNonClientDpiScaling && IsProcessPerMonitorDpiAware()) {
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
        Show(_initialShowMode);
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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = backgroundBrush;
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAINFRAME);
    wcex.lpszClassName = windowClass.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
        self = GetUserDataPtr<MainWindow *>(hWnd);
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
        self->onPaint(); return 0;
    case WM_ACTIVATE:
        self->onActivate(LOWORD(wParam) != WA_INACTIVE); break;
    case WM_SETFOCUS:
        self->onFocus(); return 0;
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
            if (::PtInRegion(self->draggable_region_, point.x, point.y))
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
        SetUserDataPtr(hWnd, self);
        self->_hwnd = hWnd;

        self->onNCCreate(cs);
    } break;

    case WM_CREATE:
        self->onCreate(reinterpret_cast<CREATESTRUCT *>(lParam)); break;

    case WM_NCDESTROY:
        // Clear the reference to |self|.
        SetUserDataPtr(hWnd, nullptr);
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
    // if (browser_window_ && ::IsWindowEnabled(_hwnd))
    // {
    //     browser_window_->SetFocus(true);
    // }
}

void MainWindow::onActivate(bool active)
{
}

void MainWindow::onSize(bool minimized)
{
    // if (minimized)
    // {
    //     // Notify the browser window that it was hidden and do nothing further.
    //     if (browser_window_)
    //     {
    //         browser_window_->Hide();
    //     }
    //     return;
    // }

    // if (browser_window_)
    // {
    //     browser_window_->Show();
    // }

    // RECT rect;
    // GetClientRect(hwnd_, &rect);

    //     // Size the browser window to the whole client area.
    //     browser_window_->SetBounds(0, 0, rect.right, rect.bottom);
}

void MainWindow::onMove()
{
    //  RECT rect;
    // GetClientRect(hwnd_, &rect);

    //     // Size the browser window to the whole client area.
    //     browser_window_->SetBounds(0, 0, rect.right, rect.bottom);
    // // Notify the browser of move events so that popup windows are displayed
    // // in the correct location and dismissed when the window moves.
    // CefRefPtr<CefBrowser> browser = GetBrowser();
    // if (browser)
    // {
    //     browser->GetHost()->NotifyMoveOrResizeStarted();
    // }
}

void MainWindow::onDpiChanged(WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(wParam) != HIWORD(wParam))
    {
        NOTIMPLEMENTED() << "Received non-square scaling factors";
        return;
    }

    // if (browser_window_ && with_osr_)
    // {
    //     // Scale factor for the new display.
    //     const float display_scale_factor =
    //         static_cast<float>(LOWORD(wParam)) / DPI_1X;
    //     browser_window_->SetDeviceScaleFactor(display_scale_factor);
    // }

    // // Suggested size and position of the current window scaled for the new DPI.
    // const RECT *rect = reinterpret_cast<RECT *>(lParam);
    // SetBounds(rect->left, rect->top, rect->right - rect->left,
    //           rect->bottom - rect->top);
}

bool MainWindow::onEraseBkgnd()
{
    // Erase the background when the browser does not exist.
    // return (browser_window_ == nullptr);
    return false; // Do not erase the background.
}

bool MainWindow::onCommand(UINT id)
{
    // if (id >= ID_TESTS_FIRST && id <= ID_TESTS_LAST)
    // {
    //     delegate_->OnTest(this, id);
    //     return true;
    // }

    // switch (id)
    // {
    // case IDM_ABOUT:
    //     onAbout();
    //     return true;
    // case IDM_EXIT:
    //     delegate_->onExit(this);
    //     return true;

    return false;
}


void MainWindow::onAbout()
{
    // Show the about box.
    DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), _hwnd,
              AboutWndProc);
}

void MainWindow::onNCCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (IsProcessPerMonitorDpiAware())
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

void MainWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    // _windowCreated = true;
}

bool MainWindow::onClose()
{

    // Allow the close.
    return false;
}

void MainWindow::onDestroyed()
{
    _windowDestroyed = true;
}

