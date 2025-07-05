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

    // quit CEF.
    void quitCef();
    // doMessageLoopWork
    void doCefMessageLoopWork();

    void initCefSettings(CefSettings& settings);
    // Initialize CEF.
    int initCef(CefSettings& settings, bool logging);

    // Returns the application working directory.
    CefString getAppWorkingDirectory();

    // Returns the temp directory.
    CefString getAppTempDirectory();

    // Quit the application message loop.
    void AppQuitMessageLoop();
private:
    CefManager(const CefManager&) = delete;
    CefManager& operator=(const CefManager&) = delete;
};
}

#endif //!CEFMANAGER_H