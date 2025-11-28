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

namespace cefview {

struct CefConfig
{
    // Make browser process message loop run in a separate thread.
    // Macos is not supported, so we need to run messageloop
    bool multiThreadedMessageLoop = false;
    bool noSandbox = true;
    bool disableGpu = true;
    int remoteDebuggingPort = 18432;
    std::string cookieableSchemesList = "http,https";
    std::string cachePath;
    std::string resourcesDirPath;
    std::string localesDirPath;
    // Log configuration
    int logSeverity = 0; // LOGSEVERITY_DEFAULT
    std::string logFilePath;
    // Subprocess path, only valid on Windows
    std::string subProcessPath;
};

} // namespace cefview

#endif //!CEFCONFIG_H