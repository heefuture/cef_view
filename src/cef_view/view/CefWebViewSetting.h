/**
 * @file        CefWebViewSetting.h
 * @brief       Configuration settings for CefWebView.
 * @version     1.0
 * @author      heefuture
 * @date        2025.11.10
 * @copyright
 */
#ifndef CEFWEBVIEWSETTING_H
#define CEFWEBVIEWSETTING_H
#pragma once

#include <string>

namespace cefview {

// Configuration for creating a CefWebView.
struct CefWebViewSetting {
    std::string url;

    bool offScreenRenderingEnabled = false;
    int windowlessFrameRate = 60;

    // Position and size. 0 width/height fills the parent window.
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool transparentPaintingEnabled = false;
    unsigned int backgroundColor = 0x00000000;  // ARGB format
};

}  // namespace cefview

#endif // CEFWEBVIEWSETTING_H