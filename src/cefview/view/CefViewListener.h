/**
* @file        CefViewListener.h
* @brief       CefViewListener 类的定义
*              用于监听 CefViewBase 的各种事件
* @version     1.0
* @author      heefuture
* @date        2025.07.02
* @copyright
*/
#ifndef CEFVIEWLISTENER_H
#define CEFVIEWLISTENER_H
#pragma once

//#include "include/cef_load_handler.h"
//#include "include/cef_request_handler.h"
//#include "include/cef_context_menu_handler.h"
//#include "include/cef_download_handler.h"
//#include "include/cef_dialog_handler.h"

namespace cef
{
class CefViewListener {
    public:
    CefViewListener() = default;
    virtual ~CefViewListener() = default;

    virtual void onTitleChange(int browserId, const std::string& title) = 0;
    virtual void onUrlChange(int browserId, const std::string& oldUrl, const std::string& url) = 0;

    virtual void onLoadingStateChange(int browserId, bool isLoading, bool canGoBack, bool canGoForward) = 0;
    virtual void onLoadStart(const std::string& url) = 0;
    virtual void onLoadEnd(const std::string& url) = 0;
    virtual void onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl) = 0;

    virtual void onAfterCreated(int browserId) = 0;
    virtual void onBeforeClose(int browserId) = 0;

    //virtual void onBeforeDownloadEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) = 0;
    //virtual void onDownloadUpdatedEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback) = 0;
};
}

#endif //!CEFVIEWLISTENER_H