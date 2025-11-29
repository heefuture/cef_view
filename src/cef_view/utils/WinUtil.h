/**
 * @file        WinUtil.h
 * @brief       Utility class for Windows-specific operations
 * @version     1.0
 * @date        2025.11.29
 */
#pragma once

#include <windows.h>
#include <string>

#include "include/internal/cef_types_wrappers.h"

namespace cefview {

/**
 * @brief Utility class for Windows-specific operations
 */
class WinUtil {
public:
    /**
     * @brief Get current time in microseconds
     * @return Current time in microseconds
     */
    static uint64_t GetTimeNow();

    /**
     * @brief Set the window's user data pointer
     * @param hwnd Window handle
     * @param ptr Pointer to user data
     */
    static void SetUserDataPtr(HWND hwnd, void* ptr);

    /**
     * @brief Get the window's user data pointer
     * @tparam T Type to cast the pointer to
     * @param hwnd Window handle
     * @return User data pointer cast to type T
     */
    template <typename T>
    static T GetUserDataPtr(HWND hwnd) {
        return reinterpret_cast<T>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    /**
     * @brief Set the window's window procedure pointer
     * @param hwnd Window handle
     * @param wndProc New window procedure
     * @return Old window procedure pointer
     */
    static WNDPROC SetWndProcPtr(HWND hwnd, WNDPROC wndProc);

    /**
     * @brief Check if the OS is Windows 8 or newer
     * @return true if Windows 8 or newer, false otherwise
     */
    static bool IsWindows8OrNewer();

    /**
     * @brief Get resource string by ID
     * @param id Resource string ID
     * @return Resource string
     */
    static std::wstring GetResourceString(UINT id);

    /**
     * @brief Check if mouse event is from touch emulation
     * @param message Windows message
     * @return true if from touch, false otherwise
     */
    static bool IsMouseEventFromTouch(UINT message);

    /**
     * @brief Get CEF mouse modifiers from Windows message
     * @param wparam Windows WPARAM
     * @return CEF mouse modifier flags
     */
    static int GetCefMouseModifiers(WPARAM wparam);

    /**
     * @brief Get CEF keyboard modifiers from Windows message
     * @param wparam Windows WPARAM
     * @param lparam Windows LPARAM
     * @return CEF keyboard modifier flags
     */
    static int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);

    /**
     * @brief Check if a key is pressed
     * @param wparam Virtual key code
     * @return true if key is down, false otherwise
     */
    static bool IsKeyDown(WPARAM wparam);

    /**
     * @brief Check if process is per-monitor DPI aware
     * @return true if per-monitor DPI aware, false otherwise
     */
    static bool IsProcessPerMonitorDpiAware();

    /**
     * @brief Get scale factor for a specific window
     * @param hwnd Window handle
     * @return Scale factor (e.g., 2.0 for 200% scaling)
     */
    static float GetWindowScaleFactor(HWND hwnd);

    /**
     * @brief Get device scale factor
     * @return Device scale factor (e.g., 2.0 for 200% scaling)
     */
    static float GetDeviceScaleFactor();

    /**
     * @brief Get window client rectangle in logical coordinates
     * @param hwnd Window handle
     * @param deviceScaleFactor Device scale factor
     * @return Window rectangle in logical coordinates
     */
    static CefRect GetWindowRect(HWND hwnd, float deviceScaleFactor);

private:
    WinUtil() = delete;
    ~WinUtil() = delete;
    WinUtil(const WinUtil&) = delete;
    WinUtil& operator=(const WinUtil&) = delete;
};

}  // namespace cefview
