

/**
* @file        CefClientAPi.h
* @brief       CefClientAPi.h is the header file for the CefClient API, which provides functions to initialize and manage the CefClient application.
* @version     1.0
* @author      heefuture
* @date        2025.07.04
* @copyright
*/
#ifndef CEFCLIENTAPI_H
#define CEFCLIENTAPI_H
#pragma once

#include "export.h"


int WEBVIEW_EXPORTS CefClientInit(int& argc, char** argv, QtCefCommandLineOption *option, void (*settings_func)(CefSettings& settings), bool logging);
void WEBVIEW_EXPORTS CefClientQuit();
void WEBVIEW_EXPORTS CefClientDoMessageLoopWork();

#endif //!CEFCLIENTAPI_H