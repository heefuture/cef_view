#include "utils/WinUtil.h"

#include "GeometryUtil.h"

#include <shellscalingapi.h>
// #include "include/base/cef_logging.h"

namespace cefview::util {

LARGE_INTEGER qi_freq_ = {};

uint64_t getTimeNow() {
    if (!qi_freq_.HighPart && !qi_freq_.LowPart) {
        ::QueryPerformanceFrequency(&qi_freq_);
    }
    LARGE_INTEGER t = {};
    ::QueryPerformanceCounter(&t);
    return static_cast<uint64_t>((t.QuadPart / double(qi_freq_.QuadPart)) * 1000000);
}

void setUserDataPtr(HWND hWnd, void* ptr) {
    ::SetLastError(ERROR_SUCCESS);
    LONG_PTR result = ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
    // CHECK(result != 0 || ::GetLastError() == ERROR_SUCCESS);

}

WNDPROC setWndProcPtr(HWND hWnd, WNDPROC wndProc) {
    WNDPROC old = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC));
    // CHECK(old != nullptr);
    LONG_PTR result = ::SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProc));
    // CHECK(result != 0 || ::GetLastError() == ERROR_SUCCESS);
    return old;
}

std::wstring getResourceString(UINT id) {
#define MAX_LOADSTRING 100
    TCHAR buff[MAX_LOADSTRING] = {0};
    LoadString(::GetModuleHandle(nullptr), id, buff, MAX_LOADSTRING);
    return buff;
}

// Helper funtion to check if it is Windows8 or greater.
// https://msdn.microsoft.com/en-us/library/ms724833(v=vs.85).aspx
bool isWindows8OrNewer() {
    OSVERSIONINFOEX osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 2;
    DWORDLONG dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    return ::VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION,
                               dwlConditionMask) == TRUE;
}

// Helper function to detect mouse messages coming from emulation of touch
// events. These should be ignored.
bool isMouseEventFromTouch(UINT message) {
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
    return (message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST) &&
           (::GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) ==
               MOUSEEVENTF_FROMTOUCH;
}

int getCefMouseModifiers(WPARAM wparam) {
    int modifiers = 0;
    if (wparam & MK_CONTROL) {
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if (wparam & MK_SHIFT) {
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if (isKeyDown(VK_MENU)) {
        modifiers |= EVENTFLAG_ALT_DOWN;
    }
    if (wparam & MK_LBUTTON) {
        modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if (wparam & MK_MBUTTON) {
        modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    if (wparam & MK_RBUTTON) {
        modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }

    // Low bit set from GetKeyState indicates "toggled".
    if (::GetKeyState(VK_NUMLOCK) & 1) {
        modifiers |= EVENTFLAG_NUM_LOCK_ON;
    }
    if (::GetKeyState(VK_CAPITAL) & 1) {
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    }
    return modifiers;
}

int getCefKeyboardModifiers(WPARAM wparam, LPARAM lparam) {
    int modifiers = 0;
    if (isKeyDown(VK_SHIFT)) {
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if (isKeyDown(VK_CONTROL)) {
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if (isKeyDown(VK_MENU)) {
        modifiers |= EVENTFLAG_ALT_DOWN;
    }

    // Low bit set from GetKeyState indicates "toggled".
    if (::GetKeyState(VK_NUMLOCK) & 1) {
        modifiers |= EVENTFLAG_NUM_LOCK_ON;
    }
    if (::GetKeyState(VK_CAPITAL) & 1) {
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    }

    switch (wparam) {
    case VK_RETURN:
        if ((lparam >> 16) & KF_EXTENDED) {
            modifiers |= EVENTFLAG_IS_KEY_PAD;
        } break;
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
        if (!((lparam >> 16) & KF_EXTENDED)) {
            modifiers |= EVENTFLAG_IS_KEY_PAD;
        } break;
    case VK_NUMLOCK:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_DECIMAL:
    case VK_CLEAR:
        modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_SHIFT:
        if (isKeyDown(VK_LSHIFT)) {
            modifiers |= EVENTFLAG_IS_LEFT;
        }
        else if (isKeyDown(VK_RSHIFT)) {
            modifiers |= EVENTFLAG_IS_RIGHT;
        }
        break;
    case VK_CONTROL:
        if (isKeyDown(VK_LCONTROL)) {
            modifiers |= EVENTFLAG_IS_LEFT;
        }
        else if (isKeyDown(VK_RCONTROL)) {
            modifiers |= EVENTFLAG_IS_RIGHT;
        }
        break;
    case VK_MENU:
        if (isKeyDown(VK_LMENU)) {
            modifiers |= EVENTFLAG_IS_LEFT;
        }
        else if (isKeyDown(VK_RMENU)) {
            modifiers |= EVENTFLAG_IS_RIGHT;
        }
        break;
    case VK_LWIN:
        modifiers |= EVENTFLAG_IS_LEFT;
        break;
    case VK_RWIN:
        modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    }
    return modifiers;
}

bool isKeyDown(WPARAM wparam) {
    return (::GetKeyState(wparam) & 0x8000) != 0;
}

// Returns true if the process is per monitor DPI aware.
bool isProcessPerMonitorDpiAware() {
    enum class PerMonitorDpiAware
    {
        UNKNOWN = 0,
        PER_MONITOR_DPI_UNAWARE,
        PER_MONITOR_DPI_AWARE,
    };
    static PerMonitorDpiAware per_monitor_dpi_aware = PerMonitorDpiAware::UNKNOWN;
    if (per_monitor_dpi_aware == PerMonitorDpiAware::UNKNOWN) {
        per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_UNAWARE;
        HMODULE shcore_dll = ::LoadLibrary(L"shcore.dll");
        if (shcore_dll) {
            typedef HRESULT(WINAPI * GetProcessDpiAwarenessPtr)(HANDLE, PROCESS_DPI_AWARENESS *);
            GetProcessDpiAwarenessPtr func_ptr = reinterpret_cast<GetProcessDpiAwarenessPtr>(::GetProcAddress(shcore_dll, "GetProcessDpiAwareness"));
            if (func_ptr) {
                PROCESS_DPI_AWARENESS awareness;
                if (SUCCEEDED(func_ptr(nullptr, &awareness)) && awareness == PROCESS_PER_MONITOR_DPI_AWARE) {
                    per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
                }
            }
        }
    }
    return per_monitor_dpi_aware == PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
}

// DPI value for 1x scale factor.
#define DPI_1X 96.0f
float getWindowScaleFactor(HWND hwnd)
{
    if (hwnd && isProcessPerMonitorDpiAware()) {
        typedef UINT(WINAPI * GetDpiForWindowPtr)(HWND);
        static GetDpiForWindowPtr func_ptr = reinterpret_cast<GetDpiForWindowPtr>(::GetProcAddress(GetModuleHandle(L"user32.dll"), "GetDpiForWindow"));
        if (func_ptr) {
            return static_cast<float>(func_ptr(hwnd)) / DPI_1X;
        }
    }

    return getDeviceScaleFactor();
}

float getDeviceScaleFactor() {
    static float scale_factor = 1.0;
    static bool initialized = false;

    if (!initialized) {
        // This value is safe to cache for the life time of the app since the user
        // must logout to change the DPI setting. This value also applies to all
        // screens.
        HDC screen_dc = ::GetDC(nullptr);
        int dpi_x = ::GetDeviceCaps(screen_dc, LOGPIXELSX);
        scale_factor = static_cast<float>(dpi_x) / 96.0f;
        ::ReleaseDC(nullptr, screen_dc);
        initialized = true;
    }

    return scale_factor;
}

CefRect getWindowRect(HWND hwnd, float deviceScaleFactor) {
    RECT clientRect;
    ::GetClientRect(hwnd, &clientRect);

    CefRect rect;
    rect.x = rect.y = 0;

    rect.width = deviceToLogical(clientRect.right - clientRect.left, deviceScaleFactor);
    if (rect.width == 0) {
        rect.width = 1;
    }

    rect.height = deviceToLogical(clientRect.bottom - clientRect.top, deviceScaleFactor);
    if (rect.height == 0) {
        rect.height = 1;
    }

    return rect;
    // ::ClientToScreen(hwnd, (LPPOINT)&clientRect.left);
    // ::ClientToScreen(hwnd, (LPPOINT)&clientRect.right);
    // return CefRect(clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
}

}  // namespace cefview::util
