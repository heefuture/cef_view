
/**
* @file        CefView.h
* @brief       CefView 类的定义
* @version     1.0
* @author      heefuture
* @date        2025.07.04
* @copyright
*/
#pragma once
#include "CefViewBase.h"

namespace cef {

class CefView : public CefViewBase
{
public:
    CefView(HWND hwnd);
    ~CefView(void);

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
    void reCreateBrowser(int width, int height);

private:
    HWND _hwnd; // Native window handle for the CefView
};
}