#include "AcrylicHelper.h"

#include <dwmapi.h>
#include <VersionHelpers.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ntdll.lib")

namespace cefview {

// Undocumented Windows API structures for SetWindowCompositionAttribute
enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,  // Win10 1803+
    ACCENT_ENABLE_HOSTBACKDROP = 5,        // Win10 RS5+
    ACCENT_INVALID_STATE = 6
};

struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;  // ARGB format
    DWORD AnimationId;
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

// Function pointer for SetWindowCompositionAttribute
using SetWindowCompositionAttributeFunc = BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

// DWM attributes for Windows 11
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif

// System backdrop types for Windows 11 22H2+
enum DWM_SYSTEMBACKDROP_TYPE {
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,      // Mica
    DWMSBT_TRANSIENTWINDOW = 3, // Acrylic
    DWMSBT_TABBEDWINDOW = 4     // Mica Alt
};

DWORD AcrylicHelper::GetWindowsBuildNumber() {
    static DWORD buildNumber = 0;
    if (buildNumber != 0) {
        return buildNumber;
    }

    // Use RtlGetVersion to get accurate version info
    using RtlGetVersionFunc = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunc>(
            GetProcAddress(ntdll, "RtlGetVersion"));
        if (rtlGetVersion) {
            RTL_OSVERSIONINFOW osvi = {};
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (rtlGetVersion(&osvi) == 0) {
                buildNumber = osvi.dwBuildNumber;
            }
        }
    }

    return buildNumber;
}

bool AcrylicHelper::IsSupported(BackdropType type) {
    DWORD build = GetWindowsBuildNumber();

    switch (type) {
    case BackdropType::kNone:
        return true;

    case BackdropType::kAcrylic:
        // Win10 1803 (build 17134) or later
        return build >= 17134;

    case BackdropType::kMica:
    case BackdropType::kMicaAlt:
        // Win11 22H2 (build 22621) or later for proper Mica support
        return build >= 22621;

    default:
        return false;
    }
}

bool AcrylicHelper::ExtendFrameIntoClientArea(HWND hwnd) {
    // Extend the frame into the entire client area
    MARGINS margins = {-1, -1, -1, -1};
    HRESULT hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    return SUCCEEDED(hr);
}

bool AcrylicHelper::ApplyAcrylic(HWND hwnd) {
    // Get SetWindowCompositionAttribute function
    static SetWindowCompositionAttributeFunc setWindowCompositionAttribute = nullptr;
    if (!setWindowCompositionAttribute) {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            setWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeFunc>(
                GetProcAddress(user32, "SetWindowCompositionAttribute"));
        }
    }

    if (!setWindowCompositionAttribute) {
        return false;
    }

    // Configure accent policy for Acrylic effect
    // GradientColor format: AABBGGRR (alpha in high byte, then BGR)
    // Use semi-transparent dark background for better blur visibility
    ACCENT_POLICY accent = {};
    accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    accent.AccentFlags = 2;  // Draw all borders
    // 0xD0202020 = 81% alpha, dark gray tint (makes blur more visible)
    accent.GradientColor = 0xD0202020;

    WINDOWCOMPOSITIONATTRIBDATA data = {};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);

    return setWindowCompositionAttribute(hwnd, &data) != FALSE;
}

bool AcrylicHelper::ApplyMica(HWND hwnd, bool micaAlt) {
    DWORD build = GetWindowsBuildNumber();

    // Try DWMWA_SYSTEMBACKDROP_TYPE first (Win11 22H2+)
    if (build >= 22621) {
        DWM_SYSTEMBACKDROP_TYPE backdropType = micaAlt ? DWMSBT_TABBEDWINDOW : DWMSBT_MAINWINDOW;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                           &backdropType, sizeof(backdropType));
        if (SUCCEEDED(hr)) {
            return true;
        }
    }

    // Fallback for Win11 21H2 (build 22000)
    if (build >= 22000) {
        BOOL value = TRUE;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT,
                                           &value, sizeof(value));
        return SUCCEEDED(hr);
    }

    return false;
}

bool AcrylicHelper::ApplyBackdrop(HWND hwnd, BackdropType type) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    if (type == BackdropType::kNone) {
        return RemoveBackdrop(hwnd);
    }

    DWORD build = GetWindowsBuildNumber();

    // Extend frame for transparency support - must be done before setting backdrop
    if (!ExtendFrameIntoClientArea(hwnd)) {
        return false;
    }

    bool success = false;

    // On Windows 11 22H2+, use DWMWA_SYSTEMBACKDROP_TYPE for all backdrop types
    if (build >= 22621) {
        DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_NONE;
        switch (type) {
        case BackdropType::kNone:
            backdropType = DWMSBT_NONE;
            break;
        case BackdropType::kAcrylic:
            backdropType = DWMSBT_TRANSIENTWINDOW;  // Acrylic on Win11
            break;
        case BackdropType::kMica:
            backdropType = DWMSBT_MAINWINDOW;
            break;
        case BackdropType::kMicaAlt:
            backdropType = DWMSBT_TABBEDWINDOW;
            break;
        }

        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                           &backdropType, sizeof(backdropType));
        success = SUCCEEDED(hr);
    }
    // On Windows 11 21H2 (build 22000-22620)
    else if (build >= 22000) {
        // Mica/MicaAlt: use DWMWA_MICA_EFFECT
        if (type == BackdropType::kMica || type == BackdropType::kMicaAlt) {
            BOOL value = TRUE;
            HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
            success = SUCCEEDED(hr);
        }
        // Acrylic on Win11 21H2: fallback to SetWindowCompositionAttribute
        else if (type == BackdropType::kAcrylic) {
            success = ApplyAcrylic(hwnd);
        }
    }
    // On Windows 10 1803+ (build 17134-21999), use SetWindowCompositionAttribute for Acrylic
    else if (build >= 17134 && type == BackdropType::kAcrylic) {
        success = ApplyAcrylic(hwnd);
    }

    return success;
}

bool AcrylicHelper::RemoveBackdrop(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    DWORD build = GetWindowsBuildNumber();

    // Remove system backdrop (Win11 22H2+)
    if (build >= 22621) {
        DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_NONE;
        DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                              &backdropType, sizeof(backdropType));
    }

    // Remove Mica effect (Win11 21H2)
    if (build >= 22000) {
        BOOL value = FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
    }

    // Remove Acrylic effect
    static SetWindowCompositionAttributeFunc setWindowCompositionAttribute = nullptr;
    if (!setWindowCompositionAttribute) {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            setWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeFunc>(
                GetProcAddress(user32, "SetWindowCompositionAttribute"));
        }
    }

    if (setWindowCompositionAttribute) {
        ACCENT_POLICY accent = {};
        accent.AccentState = ACCENT_DISABLED;

        WINDOWCOMPOSITIONATTRIBDATA data = {};
        data.Attrib = WCA_ACCENT_POLICY;
        data.pvData = &accent;
        data.cbData = sizeof(accent);

        setWindowCompositionAttribute(hwnd, &data);
    }

    // Reset frame extension
    MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    return true;
}

}  // namespace cefview
