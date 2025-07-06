#ifndef CEFMANAGER_H
#define CEFMANAGER_H
#pragma once

#include "include/cef_app.h"
#include "export.h"

namespace cef {
class WEBVIEW_EXPORTS CefManager
{
public:
    static CefManager* getInstance();
    // Initialize CEF.
    int initCef(CefSettings& settings, bool logging);
    // quit CEF.
    void quitCef();
    // Returns the application working directory.
    CefString getAppWorkingDirectory();
    // Returns the temp directory.
    CefString getAppTempDirectory();
    // doMessageLoopWork
    void doCefMessageLoopWork();
    // quit the application message loop.
    void quitCefMessageLoop();
private:
    void initCefSettings(CefSettings& settings);
private:
    CefManager(const CefManager&) = delete;
    CefManager& operator=(const CefManager&) = delete;
};
}

#endif //!CEFMANAGER_H