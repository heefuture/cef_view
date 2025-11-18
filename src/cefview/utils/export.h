/**
* @file        Export.h
* @brief       Header file for export functionality in the webview module.
* @version     1.0
* @author      heefuture
* @date        2025.06.26
* @copyright
*/
#ifndef EXPORT_H
#define EXPORT_H
#pragma once

#ifdef WEBVIEW_BUILD_STATIC
#define WEBVIEW_EXPORTS
#else
#ifdef WEBVIEW_EXPORTS
#define WEBVIEW_EXPORTS __declspec(dllexport)
#else
#define WEBVIEW_EXPORTS __declspec(dllimport)
#endif
#endif // !IS_WEBVIEW_STATIC

#endif //!EXPORT_H