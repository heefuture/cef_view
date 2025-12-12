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

#include "OsrRenderer.h"

#if defined(_WIN32)

#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <wrl.h>

namespace cefview {

/**
 * @brief D3D11 hardware-accelerated renderer for CEF off-screen rendering mode
 *
 * Responsible for creating D3D11 device, swap chain, texture resources,
 * and rendering CEF's OnPaint/OnAcceleratedPaint buffer to window via GPU.
 */
class OsrRendererD3D11 : public OsrRenderer {
public:
    /**
     * @brief Constructor
     * @param hwnd Target window handle
     * @param width Rendering area width
     * @param height Rendering area height
     * @param transparent Enable transparent rendering
     */
    OsrRendererD3D11(HWND hwnd, int width, int height, bool transparent = false);

    /**
     * @brief Destructor, releases all D3D11 resources
     */
    ~OsrRendererD3D11() override;

    // OsrRenderer interface
    bool initialize() override;
    void uninitialize() override;
    void setBounds(int x, int y, int width, int height) override;
    void onPaint(CefRenderHandler::PaintElementType type,
                 const CefRenderHandler::RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;
    void onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList& dirtyRects,
                            const CefAcceleratedPaintInfo& info) override;
    void render() override;

protected:
    bool createDeviceAndSwapchain();
    bool createDirectComposition();
    bool createShaderResource();
    bool createSampler();
    bool createBlender();
    bool createRenderTargetView();
    void setupPipeline();
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

protected:
    HWND _hwnd = nullptr;

    // D3D11 device and context
    Microsoft::WRL::ComPtr<ID3D11Device> _d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> _d3dContext;

    // Swap chain
    Microsoft::WRL::ComPtr<IDXGISwapChain1> _swapChain;

    // DirectComposition for transparent window
    Microsoft::WRL::ComPtr<IDCompositionDevice> _dcompDevice;
    Microsoft::WRL::ComPtr<IDCompositionTarget> _dcompTarget;
    Microsoft::WRL::ComPtr<IDCompositionVisual> _dcompVisual;

    // Rendering pipeline
    Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> _samplerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> _blenderState;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetView;

    // View texture resources (for software rendering via onPaint)
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _cefViewTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _cefViewShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Buffer> _cefViewVertexBuffer;

    // Shared texture resources (for hardware rendering via onAcceleratedPaint)
    void* _sharedTextureHandle = nullptr;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> _sharedTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _sharedTextureSRV;
};

}  // namespace cefview

#endif  // defined(_WIN32)

#endif  // OSRRENDERERD3D11_H
