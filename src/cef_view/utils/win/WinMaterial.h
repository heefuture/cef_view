/**
 * @file        WinMaterial.h
 * @brief       Windows material effect helper (Acrylic/Mica)
 * @details     Provides support for Windows 10 Acrylic and Windows 11 Mica effects.
 *              Uses SetWindowCompositionAttribute for Win10 and DwmSetWindowAttribute for Win11.
 * @version     1.0
 * @author      heefuture
 * @date        2025.12.17
 * @copyright
 */
#ifndef WINMATERIAL_H
#define WINMATERIAL_H
#pragma once

#include <windows.h>

namespace cefview {

// Helper macro to construct ABGR color value
// Usage: MAKE_ABGR(alpha, red, green, blue)
// Example: MAKE_ABGR(214, 243, 243, 245) = 0xD6F5F3F3
#define MAKE_ABGR(a, r, g, b) \
    ((DWORD)(((BYTE)(b) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(r)) << 16) | (((DWORD)(BYTE)(a)) << 24)))

// Window material types
enum class MaterialType {
    kNone = 0,      // No effect
    kAcrylic,       // Windows 10/11 Acrylic blur effect
    kMica,          // Windows 11 Mica effect
    kMicaAlt        // Windows 11 Mica Alt effect
};

// Helper class for applying Windows material effects
class WinMaterial {
public:
    // Apply material effect to the specified window
    static bool ApplyMaterial(HWND hwnd, MaterialType type);

    // Apply Acrylic effect with custom color and opacity
    // colorABGR: DWORD in ABGR format (0xAABBGGRR)
    //   - AA: Alpha (0x00=transparent, 0xFF=opaque)
    //   - BB: Blue component (0x00-0xFF)
    //   - GG: Green component (0x00-0xFF)
    //   - RR: Red component (0x00-0xFF)
    // Example: 0xD6F5F3F3 = 84% opacity + RGB(243, 243, 245)
    static bool ApplyAcrylicWithColor(HWND hwnd, DWORD colorABGR);

    // Remove material effect from the specified window
    static bool RemoveMaterial(HWND hwnd);

    // Check if the specified material type is supported on current system
    static bool IsSupported(MaterialType type);

    // Get Windows build number
    static DWORD GetWindowsBuildNumber();

private:
    // Apply Acrylic effect using SetWindowCompositionAttribute (Win10+)
    static bool applyAcrylic(HWND hwnd, DWORD colorABGR);

    // Apply Mica effect using DwmSetWindowAttribute (Win11 22H2+)
    static bool applyMica(HWND hwnd, bool micaAlt);

    // Extend frame into client area for transparency
    static bool extendFrameIntoClientArea(HWND hwnd);
};

}  // namespace cefview

#endif  // !WINMATERIAL_H
