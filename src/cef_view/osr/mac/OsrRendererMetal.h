/**
 * @file OsrRendererMetal.h
 * @brief Metal hardware-accelerated off-screen renderer for CEF on macOS
 *
 * Copyright
 * Licensed under BSD-style license.
 */
#ifndef OSRRENDERERMETAL_H
#define OSRRENDERERMETAL_H
#pragma once

#include "osr/OsrRenderer.h"

#if defined(__APPLE__)

namespace cefview {

/**
 * @brief Metal-based renderer for CEF off-screen rendering on macOS
 *
 * Supports both OnPaint (software rendering via BGRA buffer upload) and
 * OnAcceleratedPaint (hardware rendering via IOSurface zero-copy binding).
 * Uses CAMetalLayer for efficient presentation to the target NSView.
 */
class OsrRendererMetal : public OsrRenderer {
public:
    /**
     * @brief Constructor
     * @param nsView Target NSView pointer (will be made layer-backed)
     * @param width Rendering area width in pixels
     * @param height Rendering area height in pixels
     * @param transparent Enable transparent rendering
     */
    OsrRendererMetal(void* nsView, int width, int height, bool transparent = false);

    /**
     * @brief Destructor, releases all Metal resources
     */
    ~OsrRendererMetal() override;

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
    void scheduleRender() override;
    void setDeviceScaleFactor(float scaleFactor) override;

private:
    /// Create Metal device, CAMetalLayer and command queue
    bool createMetalDevice();

    /// Compile shaders and create render pipeline state
    bool createRenderPipeline();

    /// Create fullscreen quad vertex buffer
    bool createVertexBuffer();

    /// Create texture sampler state
    bool createSamplerState();

    /// Create or recreate software texture for onPaint buffer upload
    bool createSoftwareTexture(int width, int height);

    /// Upload dirty regions of BGRA buffer to software texture
    void updateSoftwareTexture(const void* buffer, int width, int height,
                               const CefRenderHandler::RectList& dirtyRects);

    /// Create Metal texture from IOSurface for zero-copy rendering
    bool createTextureFromIOSurface(void* ioSurface);

    /// Apply deferred resize to CAMetalLayer
    void applyPendingResize();

    /// Release all Metal resources
    void releaseMetalResources();

private:
    void* _nsView = nullptr;             // NSView* (weak, not retained)
    void* _metalLayer = nullptr;          // CAMetalLayer*
    void* _device = nullptr;              // id<MTLDevice>
    void* _commandQueue = nullptr;        // id<MTLCommandQueue>
    void* _pipelineState = nullptr;       // id<MTLRenderPipelineState>
    void* _vertexBuffer = nullptr;        // id<MTLBuffer>
    void* _samplerState = nullptr;        // id<MTLSamplerState>

    // Software rendering texture (from onPaint BGRA buffer)
    void* _softwareTexture = nullptr;     // id<MTLTexture>
    int _softwareTextureWidth = 0;
    int _softwareTextureHeight = 0;

    // Hardware rendering texture (from onAcceleratedPaint IOSurface)
    void* _ioSurfaceTexture = nullptr;    // id<MTLTexture>
    void* _currentIOSurface = nullptr;    // IOSurfaceRef (not retained)

    bool _initialized = false;
    bool _hasPendingResize = false;
    int _pendingWidth = 0;
    int _pendingHeight = 0;
};

}  // namespace cefview

#endif  // defined(__APPLE__)

#endif  // OSRRENDERERMETAL_H
