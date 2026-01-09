/**
 * @file        WinCompositionBackdrop.cpp
 * @brief       Implementation of advanced Windows Composition backdrop effect
 */
#include "WinCompositionBackdrop.h"
#include "LogUtil.h"

#include <dwmapi.h>
#include <dcomp.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <VersionHelpers.h>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowsapp.lib")

// Windows SDK version check
#ifndef NTDDI_WIN10_RS5
#define NTDDI_WIN10_RS5 0x0A000006  // Windows 10 1809
#endif

// Include WinRT headers with proper order
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Numerics.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.Graphics.Effects.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.System.h>
#include <windows.ui.composition.interop.h>
#include <windows.graphics.effects.interop.h>
#include <DispatcherQueue.h>

namespace cefview {

namespace wf = winrt::Windows::Foundation;
namespace wu = winrt::Windows::UI;
namespace wuc = winrt::Windows::UI::Composition;
namespace wge = winrt::Windows::Graphics::Effects;
namespace ws = winrt::Windows::System;

// Windows version constants
constexpr DWORD WIN10_1809_BUILD = 17763;  // Composition API support
constexpr DWORD WIN11_21H2_BUILD = 22000;  // Mica support

// D2D1 Effect CLSIDs
static const GUID CLSID_D2D1GaussianBlur = { 0x1feb6d69, 0x2fe6, 0x4ac9, { 0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5 } };
static const GUID CLSID_D2D1Flood = { 0x61c23c20, 0xae69, 0x4d8e, { 0x94, 0xcf, 0x50, 0x07, 0x8d, 0xf6, 0x38, 0xf2 } };
static const GUID CLSID_D2D1Composite = { 0x48fc9f51, 0xf6ac, 0x48f1, { 0x8b, 0x58, 0x3b, 0x28, 0xac, 0x46, 0xf7, 0x6d } };
static const GUID CLSID_D2D1Blend = { 0x81c5b77b, 0x13f8, 0x4cdd, { 0xad, 0x20, 0xc8, 0x90, 0x54, 0x7a, 0xc6, 0x5d } };
static const GUID CLSID_D2D1Opacity = { 0x811d79a4, 0xde28, 0x4454, { 0x80, 0x94, 0xc6, 0x46, 0x85, 0xf8, 0xbd, 0x4c } };

// D2D1 Effect property indices (from d2d1effects.h)
// GaussianBlur
constexpr UINT D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION = 0;
constexpr UINT D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION = 1;
constexpr UINT D2D1_GAUSSIANBLUR_PROP_BORDER_MODE = 2;

// D2D1_GAUSSIANBLUR_OPTIMIZATION enum values
constexpr UINT D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED = 0;
constexpr UINT D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED = 1;
constexpr UINT D2D1_GAUSSIANBLUR_OPTIMIZATION_QUALITY = 2;

// Flood (ColorSource)
constexpr UINT D2D1_FLOOD_PROP_COLOR = 0;

// Opacity
constexpr UINT D2D1_OPACITY_PROP_OPACITY = 0;

// Blend
constexpr UINT D2D1_BLEND_PROP_MODE = 0;

// Base Canvas Effect class implementing IGraphicsEffectD2D1Interop
// Reference: DWMBlurGlass\DWMBlurGlassExt\Effects\CanvasEffect.hpp
struct CanvasEffect : winrt::implements<
    CanvasEffect,
    wge::IGraphicsEffect,
    wge::IGraphicsEffectSource,
    ABI::Windows::Graphics::Effects::IGraphicsEffectD2D1Interop>
{
    CLSID m_effectId{};
    winrt::hstring m_name{};
    std::unordered_map<UINT, wf::IPropertyValue> m_properties{};
    std::vector<wge::IGraphicsEffectSource> m_sources{};

    CanvasEffect(REFCLSID effectId) : m_effectId(effectId) {}

    // IGraphicsEffect
    winrt::hstring Name() { return m_name; }
    void Name(winrt::hstring const& name) { m_name = name; }

    // IGraphicsEffectD2D1Interop - The KEY interface!
    IFACEMETHOD(GetEffectId)(CLSID* id) override {
        if (!id) return E_POINTER;
        *id = m_effectId;
        return S_OK;
    }

    IFACEMETHOD(GetNamedPropertyMapping)(
        LPCWSTR name,
        UINT* index,
        ABI::Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) override {
        return E_NOTIMPL;
    }

    IFACEMETHOD(GetPropertyCount)(UINT* count) override {
        if (!count) return E_POINTER;
        *count = static_cast<UINT>(m_properties.size());
        return S_OK;
    }

    IFACEMETHOD(GetProperty)(UINT index, ABI::Windows::Foundation::IPropertyValue** value) override {
        if (!value) return E_POINTER;
        // Use at() like DWMBlurGlass - will throw if index doesn't exist
        // System should only query indices < GetPropertyCount()
        *value = m_properties.at(index).as<ABI::Windows::Foundation::IPropertyValue>().detach();
        return S_OK;
    }

    IFACEMETHOD(GetSource)(UINT index, ABI::Windows::Graphics::Effects::IGraphicsEffectSource** source) override {
        if (!source) return E_POINTER;
        // Use at() like DWMBlurGlass - will throw if index doesn't exist
        *source = m_sources.at(index).as<ABI::Windows::Graphics::Effects::IGraphicsEffectSource>().detach();
        return S_OK;
    }

    IFACEMETHOD(GetSourceCount)(UINT* count) override {
        if (!count) return E_POINTER;
        *count = static_cast<UINT>(m_sources.size());
        return S_OK;
    }

    // Helper methods
    void SetProperty(UINT index, wf::IPropertyValue const& value) {
        m_properties[index] = value;
    }

    void SetSource(UINT index, wge::IGraphicsEffectSource const& source) {
        if (index >= m_sources.size()) {
            m_sources.resize(index + 1);
        }
        m_sources[index] = source;
    }

    template<typename T>
    wf::IPropertyValue BoxValue(T value) {
        if constexpr (std::is_same_v<T, float>) {
            return winrt::Windows::Foundation::PropertyValue::CreateSingle(value).as<wf::IPropertyValue>();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return winrt::Windows::Foundation::PropertyValue::CreateUInt32(value).as<wf::IPropertyValue>();
        } else if constexpr (std::is_enum_v<T>) {
            return winrt::Windows::Foundation::PropertyValue::CreateUInt32(static_cast<uint32_t>(value)).as<wf::IPropertyValue>();
        }
        return nullptr;
    }
};

// Gaussian Blur Effect
struct GaussianBlurEffect : CanvasEffect {
    GaussianBlurEffect() : CanvasEffect(CLSID_D2D1GaussianBlur) {
        SetBlurAmount();
        SetOptimization();
        SetBorderMode();
    }

    void SetBlurAmount(float amount = 3.0f) {
        SetProperty(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, BoxValue(amount));
    }

    void SetOptimization(uint32_t optimization = D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED) {
        SetProperty(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, BoxValue(optimization));
    }

    void SetBorderMode(D2D1_BORDER_MODE mode = D2D1_BORDER_MODE_HARD) {
        SetProperty(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, BoxValue(mode));
    }

    void SetInput(wge::IGraphicsEffectSource const& source) {
        SetSource(0, source);  // Gaussian blur has only one input source at index 0
    }
};

// Color Source Effect
struct ColorSourceEffect : CanvasEffect {
    ColorSourceEffect() : CanvasEffect(CLSID_D2D1Flood) {
        SetColor();
    }

    void SetColor(wu::Color const& color = {255, 255, 255, 255}) {
        D2D1_COLOR_F d2dColor = {
            color.R / 255.0f,
            color.G / 255.0f,
            color.B / 255.0f,
            color.A / 255.0f
        };
        float values[] = { d2dColor.r, d2dColor.g, d2dColor.b, d2dColor.a };
        winrt::com_array<float> colorArray{ values, values + 4 };
        auto inspectable = winrt::Windows::Foundation::PropertyValue::CreateSingleArray(colorArray);
        SetProperty(D2D1_FLOOD_PROP_COLOR, inspectable.as<wf::IPropertyValue>());
    }
};

// Opacity Effect
struct OpacityEffect : CanvasEffect {
    OpacityEffect() : CanvasEffect(CLSID_D2D1Opacity) {
        SetOpacity();
    }

    void SetOpacity(float opacity = 1.0f) {
        SetProperty(D2D1_OPACITY_PROP_OPACITY, BoxValue(opacity));
    }

    void SetInput(wge::IGraphicsEffectSource const& source) {
        SetSource(0, source);  // Opacity has only one input source at index 0
    }
};

// Blend Effect
struct BlendEffect : CanvasEffect {
    BlendEffect() : CanvasEffect(CLSID_D2D1Blend) {
        SetMode();
    }

    void SetMode(D2D1_BLEND_MODE mode = D2D1_BLEND_MODE_MULTIPLY) {
        SetProperty(D2D1_BLEND_PROP_MODE, BoxValue(mode));
    }

    void SetBackground(wge::IGraphicsEffectSource const& source) {
        SetSource(0, source);  // D2D1_BLEND background is source 0
    }

    void SetForeground(wge::IGraphicsEffectSource const& source) {
        SetSource(1, source);  // D2D1_BLEND foreground is source 1
    }
};

// RGB and HSV color structures
struct RGB {
    double r = 0.0;  // 0.0 - 1.0
    double g = 0.0;
    double b = 0.0;
};

struct HSV {
    double h = 0.0;  // 0.0 - 360.0
    double s = 0.0;  // 0.0 - 1.0
    double v = 0.0;  // 0.0 - 1.0
};

// Color conversion utilities
RGB RgbFromColor(const wu::Color& color) {
    return RGB{
        color.R / 255.0,
        color.G / 255.0,
        color.B / 255.0
    };
}

HSV RgbToHsv(const RGB& rgb) {
    HSV hsv{};
    double maxVal = std::max(std::max(rgb.r, rgb.g), rgb.b);
    double minVal = std::min(std::min(rgb.r, rgb.g), rgb.b);
    double delta = maxVal - minVal;

    hsv.v = maxVal;

    if (delta < 0.00001) {
        hsv.s = 0.0;
        hsv.h = 0.0;
        return hsv;
    }

    if (maxVal > 0.0) {
        hsv.s = delta / maxVal;
    } else {
        hsv.s = 0.0;
        hsv.h = 0.0;
        return hsv;
    }

    if (rgb.r >= maxVal) {
        hsv.h = (rgb.g - rgb.b) / delta;
    } else if (rgb.g >= maxVal) {
        hsv.h = 2.0 + (rgb.b - rgb.r) / delta;
    } else {
        hsv.h = 4.0 + (rgb.r - rgb.g) / delta;
    }

    hsv.h *= 60.0;
    if (hsv.h < 0.0) {
        hsv.h += 360.0;
    }

    return hsv;
}

// Microsoft's TintOpacityModifier algorithm
double GetTintOpacityModifier(const wu::Color& tintColor) {
    constexpr double midPoint = 0.50;
    constexpr double whiteMaxOpacity = 0.45;
    constexpr double midPointMaxOpacity = 0.90;
    constexpr double blackMaxOpacity = 0.85;

    const RGB rgb = RgbFromColor(tintColor);
    const HSV hsv = RgbToHsv(rgb);

    double opacityModifier = midPointMaxOpacity;

    if (std::abs(hsv.v - midPoint) > 0.001) {
        double lowestMaxOpacity = midPointMaxOpacity;
        double maxDeviation = midPoint;

        if (hsv.v > midPoint) {
            lowestMaxOpacity = whiteMaxOpacity;
            maxDeviation = 1.0 - maxDeviation;
        } else {
            lowestMaxOpacity = blackMaxOpacity;
        }

        double maxOpacitySuppression = midPointMaxOpacity - lowestMaxOpacity;
        const double deviation = std::abs(hsv.v - midPoint);
        const double normalizedDeviation = deviation / maxDeviation;

        if (hsv.s > 0.0) {
            maxOpacitySuppression *= std::max(1.0 - (hsv.s * 2.0), 0.0);
        }

        const double opacitySuppression = maxOpacitySuppression * normalizedDeviation;
        opacityModifier = midPointMaxOpacity - opacitySuppression;
    }

    return opacityModifier;
}

// Get effective tint color with opacity adjustment
wu::Color GetEffectiveTintColor(wu::Color tintColor, float tintOpacity, float luminosityOpacity) {
    if (luminosityOpacity >= 0.0f) {
        tintColor.A = static_cast<BYTE>(std::round(tintColor.A * tintOpacity));
    } else {
        const double tintOpacityModifier = GetTintOpacityModifier(tintColor);
        tintColor.A = static_cast<BYTE>(std::round(tintColor.A * tintOpacity * tintOpacityModifier));
    }
    return tintColor;
}

// Get luminosity color
wu::Color GetLuminosityColor(wu::Color tintColor, float tintOpacity, float luminosityOpacity) {
    const RGB rgbTintColor = RgbFromColor(tintColor);

    if (luminosityOpacity >= 0.0f) {
        wu::Color result = tintColor;
        result.A = static_cast<BYTE>(std::round(255.0 * std::clamp(static_cast<double>(luminosityOpacity), 0.0, 1.0)));
        return result;
    }

    // Auto-calculate luminosity
    constexpr double minHsvV = 0.125;
    constexpr double maxHsvV = 0.965;

    const HSV hsvTintColor = RgbToHsv(rgbTintColor);
    const double clampedHsvV = std::clamp(hsvTintColor.v, minHsvV, maxHsvV);

    // Map tint opacity to luminosity opacity range
    constexpr double minLuminosityOpacity = 0.15;
    constexpr double maxLuminosityOpacity = 1.03;
    const double luminosityOpacityRangeMax = maxLuminosityOpacity - minLuminosityOpacity;
    const double mappedTintOpacity = (tintOpacity * luminosityOpacityRangeMax) + minLuminosityOpacity;

    wu::Color result = tintColor;
    result.A = static_cast<BYTE>(std::round(255.0 * std::min(mappedTintOpacity, 1.0)));
    return result;
}

// Implementation structure
struct WinCompositionBackdropImpl {
    ws::DispatcherQueueController dispatcherQueueController{ nullptr };
    wuc::Compositor compositor{ nullptr };
    wuc::Desktop::DesktopWindowTarget windowTarget{ nullptr };
    wuc::ContainerVisual containerVisual{ nullptr };
    wuc::SpriteVisual spriteVisual{ nullptr };
    wuc::CompositionBrush currentBrush{ nullptr };

    bool initialized{ false };

    ~WinCompositionBackdropImpl() {
        Cleanup();
    }

    void Cleanup() {
        currentBrush = nullptr;
        spriteVisual = nullptr;
        containerVisual = nullptr;
        windowTarget = nullptr;
        compositor = nullptr;
        dispatcherQueueController = nullptr;

        initialized = false;
    }

    // Brush creation helper methods
    wuc::CompositionBrush CreateAcrylicBrush(const wu::Color& tintColor, const BackdropConfig& config);
    wuc::CompositionBrush CreateMicaBrush(const wu::Color& tintColor, const BackdropConfig& config);
    wuc::CompositionBrush CreateBlurBrush(const wu::Color& tintColor, const BackdropConfig& config);
};

DWORD WinCompositionBackdrop::GetWindowsBuildNumber() {
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

bool WinCompositionBackdrop::IsSupported() {
    DWORD build = GetWindowsBuildNumber();
    return build >= WIN10_1809_BUILD;
}

WinCompositionBackdrop::WinCompositionBackdrop(HWND hwnd)
    : _impl(std::make_unique<WinCompositionBackdropImpl>())
    , _hwnd(hwnd)
    , _type(CompositionBackdropType::kNone)
    , _active(false) {
}

WinCompositionBackdrop::~WinCompositionBackdrop() {
    Remove();
}

std::unique_ptr<WinCompositionBackdrop> WinCompositionBackdrop::Create(
    HWND hwnd,
    CompositionBackdropType type,
    const BackdropConfig& config) {

    if (!hwnd || !IsWindow(hwnd)) {
        return nullptr;
    }

    if (!IsSupported()) {
        return nullptr;
    }

    auto backdrop = std::unique_ptr<WinCompositionBackdrop>(new WinCompositionBackdrop(hwnd));
    if (!backdrop->Initialize(type, config)) {
        return nullptr;
    }

    return backdrop;
}

bool WinCompositionBackdrop::Initialize(CompositionBackdropType type, const BackdropConfig& config) {
    if (_impl->initialized) {
        LOGW << "[WinCompositionBackdrop] Already initialized";
        return true;
    }

    LOGI << "[WinCompositionBackdrop] Starting initialization...";

    try {
        // Initialize WinRT
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        LOGD << "[WinCompositionBackdrop] WinRT apartment initialized";

        // Create DispatcherQueue for this thread (required for Compositor)
        DispatcherQueueOptions options{
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA
        };

        ABI::Windows::System::IDispatcherQueueController* controller = nullptr;
        HRESULT hr = CreateDispatcherQueueController(options, &controller);
        if (SUCCEEDED(hr) && controller) {
            winrt::copy_from_abi(_impl->dispatcherQueueController, controller);
            controller->Release();
            LOGD << "[WinCompositionBackdrop] DispatcherQueue created";
        } else {
            LOGE << "[WinCompositionBackdrop] CreateDispatcherQueueController failed, HRESULT=0x"
                 << std::hex << hr;
            return false;
        }

        // Create WinRT Compositor directly (application-layer approach)
        _impl->compositor = wuc::Compositor();
        if (!_impl->compositor) {
            LOGE << "[WinCompositionBackdrop] Failed to create Compositor";
            return false;
        }
        LOGD << "[WinCompositionBackdrop] Compositor created";

        // Create DesktopWindowTarget using ICompositorDesktopInterop
        auto interop = _impl->compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();
        if (!interop) {
            LOGE << "[WinCompositionBackdrop] Failed to get ICompositorDesktopInterop interface";
            return false;
        }

        hr = interop->CreateDesktopWindowTarget(
            _hwnd,
            true,  // isTopmost - CRITICAL: must be true for BackdropBrush to work!
            reinterpret_cast<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget**>(winrt::put_abi(_impl->windowTarget))
        );
        if (FAILED(hr)) {
            LOGE << "[WinCompositionBackdrop] CreateDesktopWindowTarget failed, HRESULT=0x"
                 << std::hex << hr;
            return false;
        }
        LOGD << "[WinCompositionBackdrop] DesktopWindowTarget created";

        // Create visuals
        _impl->containerVisual = _impl->compositor.CreateContainerVisual();
        _impl->spriteVisual = _impl->compositor.CreateSpriteVisual();
        LOGD << "[WinCompositionBackdrop] Visuals created";

        // Set visual properties
        _impl->containerVisual.IsVisible(true);
        _impl->spriteVisual.IsVisible(true);
        _impl->spriteVisual.RelativeSizeAdjustment({ 1.0f, 1.0f });  // Full window size

        // CRITICAL: Build Visual Tree BEFORE setting Root
        _impl->containerVisual.Children().InsertAtTop(_impl->spriteVisual);

        // Set initial size
        RECT rect{};
        GetClientRect(_hwnd, &rect);
        wf::Numerics::float2 size{
            static_cast<float>(rect.right - rect.left),
            static_cast<float>(rect.bottom - rect.top)
        };
        _impl->containerVisual.Size(size);
        // Note: spriteVisual uses RelativeSizeAdjustment, no need to set Size explicitly

        _impl->initialized = true;
        _type = type;
        _config = config;

        // Create initial brush BEFORE setting Root
        bool result = UpdateType(type) && UpdateConfig(config);

        if (!result) {
            return false;
        }

        // Set Root AFTER brush is ready - THIS IS CRITICAL!
        _impl->windowTarget.Root(_impl->containerVisual);

        LOGI << "[WinCompositionBackdrop] Initialized: Size=" << size.x << "x" << size.y
             << ", Type=" << static_cast<int>(_type);

        // WinRT Compositor auto-commits changes, no manual commit needed

        return true;

    } catch (const winrt::hresult_error& e) {
        LOGE << "[WinCompositionBackdrop] WinRT exception: 0x" << std::hex << e.code()
             << " - " << e.message().c_str();
        _impl->Cleanup();
        return false;
    } catch (const std::exception& e) {
        LOGE << "[WinCompositionBackdrop] Standard exception: " << e.what();
        _impl->Cleanup();
        return false;
    } catch (...) {
        LOGE << "[WinCompositionBackdrop] Unknown exception during initialization";
        _impl->Cleanup();
        return false;
    }
}

bool WinCompositionBackdrop::UpdateConfig(const BackdropConfig& config) {
    if (!_impl->initialized) {
        LOGE << "[WinCompositionBackdrop] UpdateConfig called but not initialized";
        return false;
    }

    LOGD << "[WinCompositionBackdrop] Updating config: tintColor=0x" << std::hex << config.tintColor
         << ", tintOpacity=" << config.tintOpacity << ", blurAmount=" << config.blurAmount;

    try {
        _config = config;

        // Convert COLORREF to wu::Color
        BYTE alpha = static_cast<BYTE>((_config.tintColor >> 24) & 0xFF);
        if (alpha == 0) {
            alpha = 255;  // Default to fully opaque if not specified
        }

        wu::Color tintColor{
            alpha,
            GetRValue(_config.tintColor),
            GetGValue(_config.tintColor),
            GetBValue(_config.tintColor)
        };

        wuc::CompositionBrush newBrush = nullptr;

        switch (_type) {
        case CompositionBackdropType::kAcrylic:
            newBrush = _impl->CreateAcrylicBrush(tintColor, _config);
            break;

        case CompositionBackdropType::kMica:
            if (GetWindowsBuildNumber() >= WIN11_21H2_BUILD) {
                newBrush = _impl->CreateMicaBrush(tintColor, _config);
            } else {
                newBrush = _impl->CreateAcrylicBrush(tintColor, _config);
            }
            break;

        case CompositionBackdropType::kBlur:
            newBrush = _impl->CreateBlurBrush(tintColor, _config);
            break;

        default:
            return false;
        }

        if (!newBrush) {
            return false;
        }

        // Apply brush with optional transition
        if (_config.enableTransition && _impl->currentBrush) {
            // TODO: Implement crossfade animation
            _impl->spriteVisual.Brush(newBrush);
        } else {
            _impl->spriteVisual.Brush(newBrush);
        }

        _impl->currentBrush = newBrush;
        _active = true;

        LOGI << "[WinCompositionBackdrop] Brush applied successfully";

        // WinRT Compositor auto-commits changes

        return true;

    } catch (const winrt::hresult_error&) {
        return false;
    }
}

wuc::CompositionBrush WinCompositionBackdropImpl::CreateAcrylicBrush(const wu::Color& tintColor, const BackdropConfig& config) {
    try {
        LOGD << "[WinCompositionBackdrop] Creating Acrylic brush...";

        // ===== TEST: Pure BackdropBrush =====
        LOGI << "[WinCompositionBackdrop] ===== TESTING PURE BACKDROPBRUSH =====";
        auto pureBackdropBrush = compositor.CreateBackdropBrush();
        if (pureBackdropBrush) {
            LOGI << "[WinCompositionBackdrop] Pure BackdropBrush created - returning directly";
            return pureBackdropBrush;
        }
        // ===== END TEST =====

        // 按照 DWMBlurGlass 的完整 Acrylic 实现
        wu::Color effectiveTint = GetEffectiveTintColor(tintColor, config.tintOpacity, config.luminosityOpacity);
        wu::Color luminosityColor = GetLuminosityColor(tintColor, config.tintOpacity, config.luminosityOpacity);

        LOGD << "[WinCompositionBackdrop] Creating effect chain...";
        LOGD << "[WinCompositionBackdrop] EffectiveTint ARGB: " 
             << (int)effectiveTint.A << "," << (int)effectiveTint.R << "," 
             << (int)effectiveTint.G << "," << (int)effectiveTint.B;
        LOGD << "[WinCompositionBackdrop] LuminosityColor ARGB: " 
             << (int)luminosityColor.A << "," << (int)luminosityColor.R << "," 
             << (int)luminosityColor.G << "," << (int)luminosityColor.B;

        // Step 1: Gaussian Blur Effect (输入为 Backdrop - CRITICAL!)
        auto gaussianBlurEffect = winrt::make_self<GaussianBlurEffect>();
        gaussianBlurEffect->Name(L"Blur");
        gaussianBlurEffect->SetBlurAmount(config.blurAmount);
        gaussianBlurEffect->SetOptimization(D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);  // DWMBlurGlass uses SPEED
        gaussianBlurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
        gaussianBlurEffect->SetInput(wuc::CompositionEffectSourceParameter{ L"Backdrop" });
        LOGD << "[WinCompositionBackdrop] GaussianBlur created with amount=" << config.blurAmount;

        // Step 2: Tint Color Source (前景颜色)
        auto tintColorEffect = winrt::make_self<ColorSourceEffect>();
        tintColorEffect->Name(L"TintColor");
        tintColorEffect->SetColor(effectiveTint);

        // Step 3: Tint Opacity Effect (控制 Tint 透明度)
        auto tintOpacityEffect = winrt::make_self<OpacityEffect>();
        tintOpacityEffect->Name(L"TintOpacity");
        tintOpacityEffect->SetOpacity(config.tintOpacity);
        tintOpacityEffect->SetInput(*tintColorEffect);

        // Step 4: Luminosity Color Source (亮度混合颜色)
        auto luminosityColorEffect = winrt::make_self<ColorSourceEffect>();
        luminosityColorEffect->Name(L"LuminosityColor");
        luminosityColorEffect->SetColor(luminosityColor);

        // Step 5: Luminosity Opacity Effect (控制 Luminosity 透明度)
        float lumOpacity = (config.luminosityOpacity >= 0.0f) ? config.luminosityOpacity : 0.65f;
        auto luminosityOpacityEffect = winrt::make_self<OpacityEffect>();
        luminosityOpacityEffect->Name(L"LuminosityOpacity");
        luminosityOpacityEffect->SetOpacity(lumOpacity);
        luminosityOpacityEffect->SetInput(*luminosityColorEffect);

        // Step 6: Luminosity Blend (COLOR 模式混合 - 注意 DWMBlurGlass 的 Bug/特性)
        auto luminosityBlendEffect = winrt::make_self<BlendEffect>();
        luminosityBlendEffect->Name(L"LuminosityBlend");
        luminosityBlendEffect->SetMode(D2D1_BLEND_MODE_COLOR);
        luminosityBlendEffect->SetBackground(*gaussianBlurEffect);
        luminosityBlendEffect->SetForeground(*luminosityOpacityEffect);

        // Step 7: Color Blend (LUMINOSITY 模式混合 - 注意 DWMBlurGlass 的 Bug/特性)
        auto colorBlendEffect = winrt::make_self<BlendEffect>();
        colorBlendEffect->Name(L"ColorBlend");
        colorBlendEffect->SetMode(D2D1_BLEND_MODE_LUMINOSITY);
        colorBlendEffect->SetBackground(*luminosityBlendEffect);
        colorBlendEffect->SetForeground(*tintOpacityEffect);

        // Note: 完整实现还需要噪声纹理层,但这需要加载噪声图片
        // 暂时跳过噪声层,如需要可以后续添加

        LOGD << "[WinCompositionBackdrop] Creating EffectFactory...";
        // Create EffectFactory and Brush
        auto effectFactory = compositor.CreateEffectFactory(colorBlendEffect.as<wge::IGraphicsEffect>());
        auto effectBrush = effectFactory.CreateBrush();

        LOGD << "[WinCompositionBackdrop] Setting Backdrop source parameter...";
        // Set Backdrop as source parameter - CRITICAL!
        effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateBackdropBrush());

        LOGI << "[WinCompositionBackdrop] Acrylic brush created successfully";
        return effectBrush;

    } catch (const winrt::hresult_error& e) {
        LOGE << "[WinCompositionBackdrop] Full Acrylic effect failed with HRESULT=0x"
             << std::hex << e.code() << ": " << e.message().c_str();

        // Fallback: 简化版模糊
        LOGW << "[WinCompositionBackdrop] Trying simplified blur";
        try {
            // Test 1: Pure BackdropBrush without any effects
            LOGD << "[WinCompositionBackdrop] Testing pure BackdropBrush...";
            auto pureBackdropBrush = compositor.CreateBackdropBrush();
            if (pureBackdropBrush) {
                LOGI << "[WinCompositionBackdrop] Pure BackdropBrush created - returning for test";
                // TEST: Enable pure BackdropBrush to verify if backdrop capture works
                return pureBackdropBrush;
            }
            
            // Test 2: Simple blur effect
            auto blurEffect = winrt::make_self<GaussianBlurEffect>();
            blurEffect->SetBlurAmount(config.blurAmount);
            blurEffect->SetOptimization(D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);
            blurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
            blurEffect->SetInput(wuc::CompositionEffectSourceParameter{ L"Backdrop" });

            auto effectFactory = compositor.CreateEffectFactory(blurEffect.as<wge::IGraphicsEffect>());
            auto effectBrush = effectFactory.CreateBrush();
            effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateBackdropBrush());

            LOGI << "[WinCompositionBackdrop] Simplified blur brush created";
            return effectBrush;
        } catch (const winrt::hresult_error& e2) {
            LOGE << "[WinCompositionBackdrop] Simplified blur also failed with HRESULT=0x"
                 << std::hex << e2.code() << ": " << e2.message().c_str();
            wu::Color fallbackColor = GetEffectiveTintColor(tintColor, config.tintOpacity, config.luminosityOpacity);
            return compositor.CreateColorBrush(fallbackColor);
        } catch (...) {
            LOGE << "[WinCompositionBackdrop] Simplified blur failed with unknown exception";
            wu::Color fallbackColor = GetEffectiveTintColor(tintColor, config.tintOpacity, config.luminosityOpacity);
            return compositor.CreateColorBrush(fallbackColor);
        }
    } catch (...) {
        LOGE << "[WinCompositionBackdrop] Full Acrylic effect failed with unknown exception";
        wu::Color fallbackColor = GetEffectiveTintColor(tintColor, config.tintOpacity, config.luminosityOpacity);
        return compositor.CreateColorBrush(fallbackColor);
    }
}

wuc::CompositionBrush WinCompositionBackdropImpl::CreateMicaBrush(const wu::Color& tintColor, const BackdropConfig& config) {
    try {
        // Try to use TryCreateBlurredWallpaperBackdropBrush (Win11+)
        auto wallpaperBrush = compositor.TryCreateBlurredWallpaperBackdropBrush();
        if (wallpaperBrush) {
            // Apply tint overlay
            wu::Color effectiveTint = GetEffectiveTintColor(tintColor, config.tintOpacity, config.luminosityOpacity);

            // Create blend effect to combine wallpaper with tint
            auto colorEffect = winrt::make_self<ColorSourceEffect>();
            colorEffect->SetColor(effectiveTint);

            auto blendEffect = winrt::make_self<BlendEffect>();
            blendEffect->SetMode(D2D1_BLEND_MODE_MULTIPLY);
            blendEffect->SetBackground(wuc::CompositionEffectSourceParameter{ L"Wallpaper" });
            blendEffect->SetForeground(*colorEffect);

            auto effectFactory = compositor.CreateEffectFactory(*blendEffect);
            auto effectBrush = effectFactory.CreateBrush();
            effectBrush.SetSourceParameter(L"Wallpaper", wallpaperBrush);

            return effectBrush;
        }
    } catch (...) {
        // Fall back to acrylic
    }

    return CreateAcrylicBrush(tintColor, config);
}

wuc::CompositionBrush WinCompositionBackdropImpl::CreateBlurBrush(const wu::Color& tintColor, const BackdropConfig& config) {
    try {
        // Create blur effect with custom parameters
        auto gaussianBlurEffect = winrt::make_self<GaussianBlurEffect>();
        gaussianBlurEffect->SetBlurAmount(config.blurAmount);
        gaussianBlurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
        gaussianBlurEffect->SetInput(wuc::CompositionEffectSourceParameter{ L"Backdrop" });

        auto effectFactory = compositor.CreateEffectFactory(*gaussianBlurEffect);
        auto effectBrush = effectFactory.CreateBrush();
        effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateBackdropBrush());

        return effectBrush;

    } catch (...) {
        wu::Color fallbackColor = GetEffectiveTintColor(tintColor, config.tintOpacity, config.luminosityOpacity);
        return compositor.CreateColorBrush(fallbackColor);
    }
}

bool WinCompositionBackdrop::UpdateType(CompositionBackdropType type) {
    if (!_impl->initialized) {
        return false;
    }

    _type = type;
    return UpdateConfig(_config);
}

bool WinCompositionBackdrop::Update() {
    if (!_impl->initialized || !_active) {
        return false;
    }

    try {
        RECT rect{};
        GetClientRect(_hwnd, &rect);
        wf::Numerics::float2 size{
            static_cast<float>(rect.right - rect.left),
            static_cast<float>(rect.bottom - rect.top)
        };

        _impl->containerVisual.Size(size);
        _impl->spriteVisual.Size(size);

        // WinRT Compositor auto-commits changes

        return true;
    } catch (const winrt::hresult_error&) {
        return false;
    }
}

bool WinCompositionBackdrop::Remove() {
    if (!_impl->initialized) {
        return false;
    }

    try {
        if (_impl->spriteVisual) {
            _impl->spriteVisual.Brush(nullptr);
        }
        _impl->Cleanup();
        _active = false;
        return true;
    } catch (const winrt::hresult_error&) {
        return false;
    }
}

const BackdropConfig& WinCompositionBackdrop::GetConfig() const {
    return _config;
}

CompositionBackdropType WinCompositionBackdrop::GetType() const {
    return _type;
}

bool WinCompositionBackdrop::IsActive() const {
    return _active && _impl->initialized;
}

}  // namespace cefview
