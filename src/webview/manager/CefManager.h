#ifndef CEFMANAGER_H
#define CEFMANAGER_H
#pragma once

#include "export.h"

class ClientApp;
class CefCommandLine;

class WEBVIEW_EXPORTS CefManager
{

// Initialize CEF.
//int CefInit(int &argc, char **argv, QtCefCommandLineOption *option, void (*settings_func)(CefSettings& settings), bool logging);

// Quit CEF.
void CefQuit();


// Quit CEF until all browser windows have closed.
void CefQuitUntilAllBrowserClosed();

// Returns the application working directory.
CefString AppGetWorkingDirectory();

// Returns the temp directory.
CefString AppGetTempDirectory();

// Initialize the application command line.
void AppInitCommandLine(int argc, const char* const* argv);

// Returns the application command line object.
CefRefPtr<CefCommandLine> AppGetCommandLine();

//// Returns the application settings based on command line arguments.
//void AppGetSettings(CefSettings& settings);

// Returns true if off-screen rendering is enabled via the command line argument.
bool AppIsOffScreenRenderingEnabled();

// Returns true if transparent painting is enabled via the command line argument.
bool AppIsTransparentPaintingEnabled();

// Quit the application message loop.
void AppQuitMessageLoop();

// add a para: g_handler
// Notify all browser windows have closed.
void NotifyAllBrowserClosed(CefRefPtr<ClientHandler> g_handler);
};


#endif //!CEFMANAGER_H