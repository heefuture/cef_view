/**
 * @file OsrRenderer.h
 * @brief Off-screen renderer base class for CEF rendering
 *
 * This file is part of CefView project.
 * Licensed under BSD-style license.
 */
#ifndef OSRRENDERER_H
#define OSRRENDERER_H
#pragma once

#include "include/cef_render_handler.h"

namespace cefview {

/**
 * @brief Abstract base class for off-screen renderers
 *
 * Defines the common interface for CEF off-screen rendering implementations.
 * Derived classes provide platform-specific rendering (D3D11, OpenGL).
 */
class OsrRenderer {
public:
    virtual ~OsrRenderer() = default;

    // Disable copy and move
    OsrRenderer(const OsrRenderer&) = delete;
    OsrRenderer& operator=(const OsrRenderer&) = delete;
    OsrRenderer(OsrRenderer&&) = delete;
    OsrRenderer& operator=(OsrRenderer&&) = delete;

    /**
     * @brief Initialize renderer resources
     * @return true on success, false on failure
     */
    virtual bool initialize() = 0;

    /**
     * @brief Release all renderer resources
     */
    virtual void uninitialize() = 0;

    /**
     * @brief Update renderer bounds
     * @param x Left position
     * @param y Top position
     * @param width Width in pixels
     * @param height Height in pixels
     */
    virtual void setBounds(int x, int y, int width, int height) = 0;

    /**
     * @brief Handle CEF OnPaint callback (software rendering)
     * @param type PET_VIEW or PET_POPUP
     * @param dirtyRects Dirty rectangles to update
     * @param buffer Pixel buffer (BGRA format)
     * @param width Buffer width
     * @param height Buffer height
     */
    virtual void onPaint(CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height) = 0;

    /**
     * @brief Handle CEF OnAcceleratedPaint callback (hardware rendering)
     * @param type PET_VIEW or PET_POPUP
     * @param dirtyRects Dirty rectangles
     * @param info Accelerated paint info (shared texture handle)
     */
    virtual void onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                    const CefRenderHandler::RectList& dirtyRects,
                                    const CefAcceleratedPaintInfo& info) = 0;

    /**
     * @brief Render current frame to window
     */
    virtual void render() = 0;

    // Accessors
    int viewX() const { return _viewX; }
    int viewY() const { return _viewY; }
    int viewWidth() const { return _viewWidth; }
    int viewHeight() const { return _viewHeight; }
    bool isTransparent() const { return _transparent; }

protected:
    explicit OsrRenderer(bool transparent);

    bool _transparent = false;
    int _viewX = 0;
    int _viewY = 0;
    int _viewWidth = 0;
    int _viewHeight = 0;
};

}  // namespace cefview

#endif  // OSRRENDERER_H
