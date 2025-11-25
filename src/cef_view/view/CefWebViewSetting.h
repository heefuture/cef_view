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

#include <string>

namespace cefview {


struct CefWebViewSetting
{
    std::string url;
    // 离屏模式相关配置(仅当renderMode=kOffScreenD3D11时有效)
    bool offScreenRenderingEnabled = false;  // 向后兼容字段,已弃用,请使用renderMode
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