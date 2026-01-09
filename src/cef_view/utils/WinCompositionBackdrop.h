/**
 * @file        WinCompositionBackdrop.h
 * @brief       Advanced Windows Composition API based backdrop effect
 * @details     Provides high-quality Acrylic/Mica effects using WinRT Composition API
 *              and DirectComposition. Supports custom tint color, opacity, blur amount
 *              and luminosity control.
 * @version     1.0
 * @author      heefuture
 * @date        2025.01.08
 * @copyright
 */
#ifndef WINCOMPOSITIONBACKDROP_H
#define WINCOMPOSITIONBACKDROP_H
#pragma once

#include <windows.h>
#include <memory>

namespace cefview {

// Forward declarations for implementation details
struct WinCompositionBackdropImpl;

// Backdrop effect types
enum class CompositionBackdropType {
    kNone = 0,      // No effect
    kAcrylic,       // Acrylic blur effect with tint and luminosity blend
    kMica,          // Mica effect (blurred wallpaper backdrop)
    kBlur           // Simple Gaussian blur
};

// Backdrop configuration
struct BackdropConfig {
    // Tint color (RGB format, alpha in high byte for custom opacity)
    COLORREF tintColor = 0xFFF3F3F5;           // Default: light gray RGB(243, 243, 245)
    
    // Tint opacity (0.0 = fully transparent, 1.0 = fully opaque)
    float tintOpacity = 0.8f;                   // Default: 80%
    
    // Luminosity opacity (controls background saturation/brightness)
    // Set to negative value to use auto-calculated value based on tint color
    float luminosityOpacity = -1.0f;            // Default: auto
    
    // Blur amount in pixels
    float blurAmount = 30.0f;                   // Default: 30px
    
    // Noise texture opacity (adds subtle texture, 0.0-1.0)
    float noiseOpacity = 0.02f;                 // Default: 2%
    
    // Enable smooth transition animation when changing brushes
    bool enableTransition = true;
    
    // Transition duration in milliseconds
    int transitionDuration = 167;               // Default: ~6 frames at 60fps
    
    BackdropConfig() = default;
    
    // Helper constructor with ARGB color (0xAARRGGBB)
    static BackdropConfig FromARGB(DWORD argb, float blur = 30.0f, float luminosity = -1.0f) {
        BackdropConfig config;
        // Convert ARGB (0xAARRGGBB) to COLORREF (0xAABBGGRR)
        // Extract components
        BYTE a = (argb >> 24) & 0xFF;
        BYTE r = (argb >> 16) & 0xFF;
        BYTE g = (argb >> 8) & 0xFF;
        BYTE b = argb & 0xFF;
        // Construct COLORREF with RGB macro + alpha in high byte
        config.tintColor = RGB(r, g, b) | (static_cast<DWORD>(a) << 24);
        config.tintOpacity = a / 255.0f;
        config.blurAmount = blur;
        config.luminosityOpacity = luminosity;
        return config;
    }
};

/**
 * @brief Advanced Windows Composition backdrop effect
 * 
 * This class provides high-quality backdrop effects using WinRT Composition API
 * and DirectComposition, similar to Windows 11's system backdrop materials.
 * 
 * Features:
 * - Full Acrylic effect with tint and luminosity blend
 * - Mica effect using blurred wallpaper backdrop
 * - Custom blur amount and color
 * - Smooth transitions between states
 * - Noise texture support
 * - Automatic opacity adjustment based on color luminosity
 * 
 * Requirements:
 * - Windows 10 1809+ (Build 17763) for basic Composition API
 * - Windows 11 21H2+ (Build 22000) for Mica effect
 * - Requires linking: dwmapi.lib, dcomp.lib, windowsapp.lib
 */
class WinCompositionBackdrop {
public:
    /**
     * @brief Create backdrop effect for a window
     * @param hwnd Window handle
     * @param type Backdrop effect type
     * @param config Backdrop configuration
     * @return Backdrop instance, or nullptr on failure
     */
    static std::unique_ptr<WinCompositionBackdrop> Create(
        HWND hwnd,
        CompositionBackdropType type,
        const BackdropConfig& config = BackdropConfig()
    );
    
    ~WinCompositionBackdrop();
    
    // Non-copyable
    WinCompositionBackdrop(const WinCompositionBackdrop&) = delete;
    WinCompositionBackdrop& operator=(const WinCompositionBackdrop&) = delete;
    
    /**
     * @brief Update backdrop configuration
     * @param config New configuration
     * @return true on success
     */
    bool UpdateConfig(const BackdropConfig& config);
    
    /**
     * @brief Update backdrop type
     * @param type New backdrop type
     * @return true on success
     */
    bool UpdateType(CompositionBackdropType type);
    
    /**
     * @brief Update backdrop for window size/state changes
     * @return true on success
     */
    bool Update();
    
    /**
     * @brief Remove backdrop effect
     * @return true on success
     */
    bool Remove();
    
    /**
     * @brief Check if WinRT Composition API is supported
     * @return true if supported
     */
    static bool IsSupported();
    
    /**
     * @brief Get Windows build number
     * @return Build number
     */
    static DWORD GetWindowsBuildNumber();
    
    /**
     * @brief Get current configuration
     * @return Current backdrop config
     */
    const BackdropConfig& GetConfig() const;
    
    /**
     * @brief Get current backdrop type
     * @return Current type
     */
    CompositionBackdropType GetType() const;
    
    /**
     * @brief Check if backdrop is active
     * @return true if active
     */
    bool IsActive() const;

private:
    WinCompositionBackdrop(HWND hwnd);
    
    bool Initialize(CompositionBackdropType type, const BackdropConfig& config);
    
    std::unique_ptr<WinCompositionBackdropImpl> _impl;
    HWND _hwnd;
    CompositionBackdropType _type;
    BackdropConfig _config;
    bool _active;
};

}  // namespace cefview

#endif  // !WINCOMPOSITIONBACKDROP_H
