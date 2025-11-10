/**
* @file        CefWebViewSetting.h
* @brief
* @version     1.0
* @author      heefuture
* @date        2025.11.10
* @copyright
*/
#ifndef CEFWEBVIEWSETTING_H
#define CEFWEBVIEWSETTING_H
#pragma once

namespace cefview {

struct CefWebViewSetting
{
    // 离屏模式
    bool offScreenRenderingEnabled = false;
    int windowlessFrameRate = 60;

    // size and position
    // size 为 0 的时候表示填充父窗体的大小
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    // 是否启用透明背景
    bool transparentPaintingEnabled = false;
    unsigned int backgroundColor = 0x00000000; // BackgroundColor in ARGB format

};


}



#endif //!CEFWEBVIEWSETTING_H