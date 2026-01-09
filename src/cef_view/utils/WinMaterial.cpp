#include "WinMaterial.h"

#include <dwmapi.h>
#include <VersionHelpers.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ntdll.lib")

namespace cefview {

// Windows version constants
#define WIN10_1803_BUILD 17134  // Windows 10 1803 - Acrylic support
#define WIN11_21H2_BUILD 22000  // Windows 11 21H2 - Mica support
#define WIN11_22H2_BUILD 22621  // Windows 11 22H2 - Enhanced backdrop API

// DWM window attributes (from dwmapi.h)
#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

// DWM system backdrop types
enum DWM_SYSTEMBACKDROP_TYPE {
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,      // Mica
    DWMSBT_TRANSIENTWINDOW = 3, // Acrylic
    DWMSBT_TABBEDWINDOW = 4     // Mica Alt
};

// SetWindowCompositionAttribute structures (undocumented API)
enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_INVALID_STATE = 5
};

struct ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

typedef BOOL(WINAPI* SetWindowCompositionAttributeFunc)(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA*);

DWORD WinMaterial::GetWindowsBuildNumber() {
    typedef LONG(WINAPI* RtlGetVersionFunc)(PRTL_OSVERSIONINFOW);
    static RtlGetVersionFunc rtlGetVersion = nullptr;

    if (!rtlGetVersion) {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll) {
            rtlGetVersion = reinterpret_cast<RtlGetVersionFunc>(
                GetProcAddress(ntdll, "RtlGetVersion"));
        }
    }

    if (rtlGetVersion) {
        RTL_OSVERSIONINFOW osvi = {};
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        if (rtlGetVersion(&osvi) == 0) {
            return osvi.dwBuildNumber;
        }
    }

    return 0;
}

bool WinMaterial::IsSupported(MaterialType type) {
    DWORD build = GetWindowsBuildNumber();

    switch (type) {
    case MaterialType::kNone:
        return true;

    case MaterialType::kAcrylic:
        return build >= WIN10_1803_BUILD;

    case MaterialType::kMica:
    case MaterialType::kMicaAlt:
        return build >= WIN11_21H2_BUILD;

    default:
        return false;
    }
}

bool WinMaterial::extendFrameIntoClientArea(HWND hwnd) {
    MARGINS margins = {-1, -1, -1, -1};
    HRESULT hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    return SUCCEEDED(hr);
}

bool WinMaterial::applyAcrylic(HWND hwnd, DWORD colorABGR) {
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
    //ACCENT_POLICY accent = {};
    //accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    //accent.AccentFlags = 2;  // Draw all borders
    //accent.GradientColor = colorABGR;


    ACCENT_POLICY accent = { ACCENT_ENABLE_BLURBEHIND, 0, 0, 0 };

    WINDOWCOMPOSITIONATTRIBDATA data = {};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &accent;
    data.cbData = sizeof(accent);

    return setWindowCompositionAttribute(hwnd, &data) != FALSE;
}

bool WinMaterial::ApplyAcrylicWithColor(HWND hwnd, DWORD colorABGR) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    // Extend frame for transparency support
    if (!extendFrameIntoClientArea(hwnd)) {
        return false;
    }

    return applyAcrylic(hwnd, colorABGR);
}

bool WinMaterial::applyMica(HWND hwnd, bool micaAlt) {
    DWORD build = GetWindowsBuildNumber();

    // Windows 11 22H2+ uses DWMWA_SYSTEMBACKDROP_TYPE
    if (build >= WIN11_22H2_BUILD) {
        DWM_SYSTEMBACKDROP_TYPE backdropType = micaAlt ? DWMSBT_TABBEDWINDOW : DWMSBT_MAINWINDOW;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                           &backdropType, sizeof(backdropType));
        return SUCCEEDED(hr);
    }
    // Windows 11 21H2 uses DWMWA_MICA_EFFECT
    else if (build >= WIN11_21H2_BUILD) {
        BOOL value = TRUE;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
        return SUCCEEDED(hr);
    }

    return false;
}

bool WinMaterial::ApplyMaterial(HWND hwnd, MaterialType type) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    if (type == MaterialType::kNone) {
        return RemoveMaterial(hwnd);
    }

    DWORD build = GetWindowsBuildNumber();

    // Extend frame for transparency support - must be done before setting material
    if (!extendFrameIntoClientArea(hwnd)) {
        return false;
    }

    bool success = false;

    // Default Acrylic color: 84% opacity + RGB(243, 243, 245)
    const DWORD defaultAcrylicColor = 0xD6F5F3F3;

    // On Windows 11 22H2+, use DWMWA_SYSTEMBACKDROP_TYPE for all backdrop types
    if (build >= 22621) {
        DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_NONE;
        switch (type) {
        case MaterialType::kNone:
            backdropType = DWMSBT_NONE;
            break;
        case MaterialType::kAcrylic:
            backdropType = DWMSBT_TRANSIENTWINDOW;  // System Acrylic on Win11
            break;
        case MaterialType::kMica:
            backdropType = DWMSBT_MAINWINDOW;
            break;
        case MaterialType::kMicaAlt:
            backdropType = DWMSBT_TABBEDWINDOW;
            break;
        default:
            break;
        }

        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                           &backdropType, sizeof(backdropType));
        success = SUCCEEDED(hr);
    }
    // On Windows 11 21H2 (build 22000-22620)
    else if (build >= 22000) {
        // Mica/MicaAlt: use DWMWA_MICA_EFFECT
        if (type == MaterialType::kMica || type == MaterialType::kMicaAlt) {
            BOOL value = TRUE;
            HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
            success = SUCCEEDED(hr);
        }
        // Acrylic on Win11 21H2: fallback to SetWindowCompositionAttribute
        else if (type == MaterialType::kAcrylic) {
            success = applyAcrylic(hwnd, defaultAcrylicColor);
        }
    }
    // On Windows 10 1803+ (build 17134-21999), use SetWindowCompositionAttribute for Acrylic
    else if (build >= 17134 && type == MaterialType::kAcrylic) {
        success = applyAcrylic(hwnd, defaultAcrylicColor);
    }

    return success;
}

bool WinMaterial::RemoveMaterial(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    DWORD build = GetWindowsBuildNumber();
    bool success = false;

    // Try Windows 11 22H2+ method first
    if (build >= WIN11_22H2_BUILD) {
        DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_NONE;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                           &backdropType, sizeof(backdropType));
        success = SUCCEEDED(hr);
    }
    // Try Windows 11 21H2 method
    else if (build >= WIN11_21H2_BUILD) {
        BOOL value = FALSE;
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
        success = SUCCEEDED(hr);
    }

    // Also try to remove SetWindowCompositionAttribute effect
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

        success = setWindowCompositionAttribute(hwnd, &data) != FALSE || success;
    }

    return success;
}

}  // namespace cefview
