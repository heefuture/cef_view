// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

#ifndef CEF_OSR_RENDERER_H_
#define CEF_OSR_RENDERER_H_
#pragma once

#include <vector>

#include "include/cef_browser.h"
#include "include/cef_render_handler.h"
#include "tests/cefclient/browser/osr_renderer_settings.h"

// Enable shader-based rendering for Linux only. Windows still uses OpenGL 1.1
// to avoid the added complexity of linking newer OpenGL APIs on that platform.
// MacOS has deprecated OpenGL and we should eventually provide a Metal-based
// implementation on that platform.

class OsrRenderer
{
public:
    explicit OsrRenderer(bool transparent = false);
    ~OsrRenderer();

    // Initialize the OpenGL environment.
    void Initialize();

    // Clean up the OpenGL environment.
    void Cleanup();

    // Render to the screen.
    void Render();

    // Forwarded from CefRenderHandler callbacks.
    void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show);
    // |rect| must be in pixel coordinates.
    void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect);
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 CefRenderHandler::PaintElementType type,
                 const CefRenderHandler::RectList &dirtyRects,
                 const void *buffer,
                 int width,
                 int height);

    // Used when rendering with shared textures.
    void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser,
                            CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList &dirtyRects,
                            unsigned int io_surface_tex,
                            int width,
                            int height);

    // Apply spin.
    void SetSpin(float spinX, float spinY);
    void IncrementSpin(float spinDX, float spinDY);

    int GetViewWidth() const { return view_width_; }
    int GetViewHeight() const { return view_height_; }

    CefRect popup_rect() const { return popup_rect_; }
    CefRect original_popup_rect() const { return original_popup_rect_; }

    void ClearPopupRects();

private:
    CefRect GetPopupRectInWebView(const CefRect &original_rect);

    bool transparent_ = false;
    bool initialized_ = false;
    unsigned int texture_id_ = 0;
    int view_width_ = 0;
    int view_height_ = 0;
    CefRect popup_rect_;
    CefRect original_popup_rect_;
    float spin_x_ = 0;
    float spin_y_ = 0;
    CefRect update_rect_;
    bool show_update_rect_ = false;
    DISALLOW_COPY_AND_ASSIGN(OsrRenderer);
};

#endif // CEF_TESTS_CEFCLIENT_BROWSER_OSR_RENDERER_H_
