/**
 * @file OsrRendererD3D11.h
 * @brief Direct3D 11 hardware-accelerated off-screen renderer for CEF
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#ifndef OSRRENDERERD3D11_H
#define OSRRENDERERD3D11_H
#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl.h>
#include <windows.h>

#include "include/cef_render_handler.h"

namespace cefview {

/**
 * @brief D3D11 hardware-accelerated renderer for CEF off-screen rendering mode
 *
 * Responsible for creating D3D11 device, swap chain, texture resources,
 * and rendering CEF's OnPaint buffer to window via GPU.
 * Implementation references dx11/DX11RenderBackend and libcef osr_d3d11_win.
 */
class OsrRendererD3D11 {
public:
    /**
     * @brief Constructor
     * @param hwnd Target window handle
     * @param width Rendering area width
     * @param height Rendering area height
     */
    OsrRendererD3D11(HWND hwnd, int width, int height);

    /**
     * @brief Destructor, releases all D3D11 resources
     */
    ~OsrRendererD3D11();

    // Disable copy and move
    OsrRendererD3D11(const OsrRendererD3D11&) = delete;
    OsrRendererD3D11& operator=(const OsrRendererD3D11&) = delete;
    OsrRendererD3D11(OsrRendererD3D11&&) = delete;
    OsrRendererD3D11& operator=(OsrRendererD3D11&&) = delete;

    /**
     * @brief Initialize D3D11 renderer
     * @return true on success, false on failure
     */
    bool initialize();

    /**
     * @brief Uninitialize and release all resources
     */
    void uninitialize();

    /**
     * @brief Handle window resize event
     * @param width New width
     * @param height New height
     */
    void resize(int width, int height);

    /**
     * @brief Update frame data from CEF OnPaint callback (CPU software rendering)
     * @param type Paint element type (View or Popup)
     * @param dirtyRects List of dirty rectangles requiring update
     * @param buffer Pixel buffer pointer (BGRA format)
     * @param width Buffer width in pixels
     * @param height Buffer height in pixels
     */
    void updateFrameData(CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height);

    /**
     * @brief Update shared texture from CEF OnAcceleratedPaint callback (GPU hardware-accelerated rendering)
     * @param type Paint element type (View or Popup)
     * @param info Accelerated paint info containing the shared texture handle
     */
    void updateSharedTexture(CefRenderHandler::PaintElementType type,
                             const CefAcceleratedPaintInfo& info);

    /**
     * @brief Render current frame to window
     */
    void render();

    /**
     * @brief Update popup visibility
     * @param visible true to show, false to hide
     */
    void updatePopupVisibility(bool visible);

    /**
     * @brief Update popup rectangle region
     * @param rect Popup rectangle (coordinates relative to view)
     */
    void updatePopupRect(const CefRect& rect);

protected:
    /**
     * @brief Create D3D11 device and swap chain
     * @return true on success, false on failure
     */
    bool createDeviceAndSwapchain();

    /**
     * @brief Create shader resources (textures and shader resource views)
     * @return true on success, false on failure
     */
    bool createShaderResource();

    /**
     * @brief Create sampler state
     * @return true on success, false on failure
     */
    bool createSampler();

    /**
     * @brief Create blend state
     * @return true on success, false on failure
     */
    bool createBlender();

    /**
     * @brief Create render target view
     * @return true on success, false on failure
     */
    bool createRenderTargetView();

    /**
     * @brief Setup rendering pipeline
     */
    void setupPipeline();

    /**
     * @brief Handle D3D11 device lost recovery
     */
    void handleDeviceLost();

    /**
     * @brief Create fullscreen quad vertex buffer
     * @param x Top-left X coordinate (normalized device coordinates)
     * @param y Top-left Y coordinate (normalized device coordinates)
     * @param w Width (normalized device coordinates)
     * @param h Height (normalized device coordinates)
     * @param viewWidth View width in pixels
     * @param viewHeight View height in pixels
     * @param ppBuffer Output vertex buffer pointer
     * @return true on success, false on failure
     */
    bool createQuadVertexBuffer(float x,
                                float y,
                                float w,
                                float h,
                                int viewWidth,
                                int viewHeight,
                                ID3D11Buffer** ppBuffer);

private:
    HWND _hwnd;   ///< Target window handle
    int _width;   ///< Rendering area width
    int _height;  ///< Rendering area height

    // D3D11 device and context
    Microsoft::WRL::ComPtr<ID3D11Device> _d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _d3dContext;

    // Swap chain
    Microsoft::WRL::ComPtr<IDXGISwapChain> _swapChain;

    // Rendering pipeline
    Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> _samplerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> _blenderState;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetView;

    // View texture resources
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _cefViewTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _cefViewShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Buffer> _cefViewVertexBuffer;

    // Popup texture resources
    bool _showPopup;  ///< Whether popup is visible
    CefRect _popupRect;  ///< Popup rectangle region
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _cefPopupTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _cefPopupShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Buffer> _cefPopupVertexBuffer;
};

}  // namespace cefview

#endif  // OSRRENDERERD3D11_H
