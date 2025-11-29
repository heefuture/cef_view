/**
 * @file        CefWebViewSetting.h
 * @brief       Configuration settings for CefWebView
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


struct CefWebViewSetting
{
    std::string url;

    // Off-screen rendering configuration (only valid when renderMode=kOffScreenD3D11)
    bool offScreenRenderingEnabled = false;  // Deprecated, use renderMode instead
    int windowlessFrameRate = 60;

    // Size and position
    // When size is 0, it will fill the parent window size
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    // Enable transparent background
    bool transparentPaintingEnabled = false;
    unsigned int backgroundColor = 0x00000000; // Background color in ARGB format

};


}



#endif //!CEFWEBVIEWSETTING_H