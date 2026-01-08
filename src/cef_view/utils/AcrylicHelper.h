/**
 * @file        AcrylicHelper.h
 * @brief       Windows Acrylic/Mica backdrop effect helper
 * @details     Provides support for Windows 10 Acrylic and Windows 11 Mica effects.
 *              Uses SetWindowCompositionAttribute for Win10 and DwmSetWindowAttribute for Win11.
 * @version     1.0
 * @author      heefuture
 * @date        2025.12.17
 * @copyright
 */
#ifndef ACRYLICHELPER_H
#define ACRYLICHELPER_H
#pragma once

#include <windows.h>

namespace cefview {

// Backdrop effect types
enum class BackdropType {
    kNone = 0,      // No effect
    kAcrylic,       // Windows 10/11 Acrylic blur effect
    kMica,          // Windows 11 Mica effect
    kMicaAlt        // Windows 11 Mica Alt effect
};

// Helper class for applying Windows backdrop effects
class AcrylicHelper {
public:
    // Apply backdrop effect to the specified window
    static bool ApplyBackdrop(HWND hwnd, BackdropType type);

    // Remove backdrop effect from the specified window
    static bool RemoveBackdrop(HWND hwnd);

    // Check if the specified backdrop type is supported on current system
    static bool IsSupported(BackdropType type);

    // Get Windows build number
    static DWORD GetWindowsBuildNumber();

private:
    // Apply Acrylic effect using SetWindowCompositionAttribute (Win10+)
    static bool ApplyAcrylic(HWND hwnd);

    // Apply Mica effect using DwmSetWindowAttribute (Win11 22H2+)
    static bool ApplyMica(HWND hwnd, bool micaAlt);

    // Extend frame into client area for transparency
    static bool ExtendFrameIntoClientArea(HWND hwnd);
};

}  // namespace cefview

#endif  // !ACRYLICHELPER_H
