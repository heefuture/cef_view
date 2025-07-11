
/**
* @file        CefWebView.h
* @brief       CefWebView 类的定义
*              用于创建和管理 CEF 浏览器视图的类
* @version     1.0
* @author      heefuture
* @date        2025.07.08
* @copyright   Copyright (C) 2025 Tencent. All rights reserved.
*/
#ifndef CEFWEBVIEW_H
#define CEFWEBVIEW_H
#pragma once
#include <view/CefWebViewBase.h>

#include <windows.h>
#include <string>
namespace cef {

class CefWebView : public CefWebViewBase
{
public:
    CefWebView(HWND parentHwnd);
    ~CefWebView(void);

    void init();
    void setRect(int left, int top, int width, int height);
    //virtual void HandleMessage(EventArgs& event) override;
    void setVisible(bool bVisible = true);

public:
    bool getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;

    void getViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override;

    bool getScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY) override;

    bool getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screenInfo) override;
protected:
    HWND createSubWindow(HWND parentHwnd, int x, int y,int width, int height, bool showWindow = true);

    void reCreateBrowser(int width, int height);

    void destroy();
private:
    HWND _parentHwnd; // Native window handle for the CefWebView
    HWND _hwnd; // Native window handle for the CefWebView
    std::string _className; // Class name for the CefWebView window
};
}

#endif //!CEFWEBVIEW_H