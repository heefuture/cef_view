/**
* @file        CefConfig.h
* @brief       Configuration settings for CEF
* @version     1.0
* @author      heefuture
* @date        2025.11.19
* @copyright
*/
#ifndef CEFCONFIG_H
#define CEFCONFIG_H
#pragma once

#include <string>
#include <cstdint>

namespace cefview {

struct CefConfig
{
    // Make browser process message loop run in a separate thread.
    // Macos is not supported, so we need to run messageloop
    bool multiThreadedMessageLoop = false;
    bool noSandbox = true;
    bool disableGpu = false;
    // Enable off-screen rendering (OSR) support globally.
    // Must be true if any CefWebView uses offScreenRenderingEnabled.
    bool windowlessRenderingEnabled = true;
    int remoteDebuggingPort = 18432;
    std::string cookieableSchemesList = "http,https";
    std::string cachePath;
    std::string resourcesDirPath;
    std::string localesDirPath;
    // Locale setting for CEF (e.g., "zh-CN", "en-US")
    std::string locale;
    // Accept-Language header value (e.g., "zh-CN,en-US;q=0.9,en;q=0.8")
    std::string acceptLanguageList;
    // Log configuration
    int logSeverity = 0; // LOGSEVERITY_DEFAULT
    std::string logFilePath;
    // Subprocess path, only valid on Windows
    std::string subProcessPath;
    // Background color for CEF browser (ARGB format: 0xAARRGGBB)
    // Default is white (0xFFFFFFFF)
    uint32_t backgroundColor = 0xFFFFFFFF;
};

} // namespace cefview

#endif //!CEFCONFIG_H