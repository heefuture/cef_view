/**
 * @file OsrRendererGL.h
 * @brief OpenGL 3.3 off-screen renderer for CEF
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#ifndef OSRRENDERERGL_H
#define OSRRENDERERGL_H
#pragma once

#include "OsrRenderer.h"

#if defined(WIN32)
#include <windows.h>
#elif defined(__APPLE__)
// Mac forward declarations
#else
// Linux forward declarations
#endif

namespace cefview {

/**
 * @brief OpenGL 3.3 shader-based renderer for CEF off-screen rendering mode
 *
 * Cross-platform OpenGL implementation using modern shader pipeline.
 * Supports OnPaint (software rendering) on all platforms.
 * OnAcceleratedPaint is only supported on Mac (IOSurface).
 */
class OsrRendererGL : public OsrRenderer {
public:
#if defined(WIN32)
    using NativeWindowHandle = HWND;
#elif defined(__APPLE__)
    using NativeWindowHandle = void*;  // NSView*
#else
    using NativeWindowHandle = unsigned long;  // X11 Window
#endif

    /**
     * @brief Constructor
     * @param hwnd Target window handle
     * @param width Rendering area width
     * @param height Rendering area height
     * @param transparent Enable transparent rendering
     */
    OsrRendererGL(NativeWindowHandle hwnd, int width, int height, bool transparent = false);

    /**
     * @brief Destructor, releases all OpenGL resources
     */
    ~OsrRendererGL() override;

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
    bool createGLContext();
    void destroyGLContext();
    bool createShaderProgram();
    bool createTexture();
    void makeCurrent();
    void swapBuffers();

private:
#if defined(WIN32)
    HWND _hwnd = nullptr;
    HDC _hdc = nullptr;
    HGLRC _hglrc = nullptr;
#elif defined(__APPLE__)
    void* _nsView = nullptr;
    void* _nsGLContext = nullptr;
#else
    unsigned long _window = 0;
    void* _display = nullptr;
    void* _glxContext = nullptr;
#endif

    // OpenGL 3.3 resources
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _textureId = 0;
    unsigned int _shaderProgram = 0;
    int _transformLoc = -1;

    bool _initialized = false;
};

}  // namespace cefview

#endif  // OSRRENDERERGL_H
