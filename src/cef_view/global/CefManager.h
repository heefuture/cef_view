#ifndef CEFMANAGER_H
#define CEFMANAGER_H
#pragma once

#include "include/cef_app.h"

#include <global/CefConfig.h>
#include <utils/export.h>

namespace cefview {

class WEBVIEW_EXPORTS CefManager
{
public:
    static CefManager* getInstance();

    // Initialize CEF.
    int initCef(const CefConfig& config);
    // quit CEF.
    void quitCef();
    // doMessageLoopWork
    void doCefMessageLoopWork();
    // runCefMessageLoop
    void runCefMessageLoop();
    // quit the application message loop.
    void quitCefMessageLoop();
private:
    CefSettings initCefSettings(const CefConfig& config);
    // Returns the application working directory.
    CefString getAppWorkingDirectory();
    // Returns the temp directory.
    CefString getAppTempDirectory();
private:
    CefManager() = default;
    CefManager(const CefManager&) = delete;
    CefManager& operator=(const CefManager&) = delete;
};
}

#endif //!CEFMANAGER_H