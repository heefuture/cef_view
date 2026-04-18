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

#include <cstdint>
#include <string>

namespace cefview {

// Configuration for CefContext initialization.
struct CefConfig {
    // Run browser UI thread on a separate thread (Windows/Linux only).
    // NOTE: Must be false in the following cases:
    //   - macOS: multi-threaded message loop is not supported by CEF.
    //   - OSR mode: breaks drag-and-drop (confirmed by CEF official docs).
    // When false, the caller must drive the loop via CefRunMessageLoop()
    // or CefDoMessageLoopWork().
    bool multiThreadedMessageLoop = false;

    // Enable off-screen rendering (OSR) globally. Must be true if any CefWebView uses OSR.
    bool windowlessRenderingEnabled = true;
    bool disableGpu = false;
    uint32_t backgroundColor = 0xFFFFFFFF;  // ARGB format

    std::string cachePath;
    std::string resourcesDirPath;
    std::string localesDirPath;
    std::string subProcessPath;  // Windows only
    // Locale setting for CEF (e.g., "zh-CN", "en-US")
    std::string locale;
    // Accept-Language header value (e.g., "zh-CN,en-US;q=0.9,en;q=0.8")
    std::string acceptLanguageList;

    std::string cookieableSchemesList = "http,https";

    // Custom product token appended to User-Agent (e.g. "ArdotDesktop/1.0.0")
    std::string userAgentProduct;

    int remoteDebuggingPort = 18432;  // 0 to disable
    // Log configuration
    int logSeverity = 0;              // Maps to cef_log_severity_t
    std::string logFilePath;

    bool noSandbox = true;
};

} // namespace cefview

#endif //!CEFCONFIG_H
