/**
* @file        CefViewListener.h
* @brief       CefWebViewListener 类的定义
* @version     1.0
* @author      heefuture
* @date        2025.07.08
* @copyright
*/
#ifndef CEFVIEWLISTENER_H
#define CEFVIEWLISTENER_H
#pragma once
#include <string>
namespace cefview
{
class CefWebViewListener {
    public:
    CefWebViewListener() = default;
    virtual ~CefWebViewListener() = default;

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