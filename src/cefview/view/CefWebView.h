
/**
* @file        CefWebView.h
* @brief       CefWebView 类的定义
*              用于创建和管理 CEF 浏览器视图的类
* @version     1.0
* @author      heefuture
* @date        2025.07.08
* @copyright
*/
#ifndef CEFWEBVIEW_H
#define CEFWEBVIEW_H
#pragma once

#include <atlcomcli.h>
#include <functional>
#include <memory>
#include <string>
#include <windows.h>

#include "CefWebViewSetting.h"

#pragma region cef_headers
#include <include/cef_base.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>
#pragma endregion // cef_headers

namespace cefview {


class CefViewClientDelegate;
class CefViewClient;
class OsrRendererD3D11;
class OsrDragEvents;
class OsrDropTargetWin;
class OsrImeHandlerWin;
class CefWebView : public std::enable_shared_from_this<CefWebView>
{
public:
    CefWebView(const std::string& url, const CefWebViewSetting& settings, HWND parentHwnd);
    ~CefWebView();

    // 禁用拷贝和移动
    CefWebView(const CefWebView&) = delete;
    CefWebView& operator=(const CefWebView&) = delete;
    CefWebView(CefWebView&&) = delete;
    CefWebView& operator=(CefWebView&&) = delete;

    void init();
    /**
     * @brief 设置浏览器的视图
     * @param[in] left 新的左边位置
     * @param[in] top 新的上边位置
     * @param[in] width 新的宽度
     * @param[in] height 新的高度
     */
    void setRect(int left, int top, int width, int height);

    void setVisible(bool bVisible = true);

    /**
     * @brief 加载一个地址
     * @param[in] url 网站地址
     */
    void loadUrl(const std::string& url);

    /**
     * @brief 获取页面 URL
     * @return 返回 URL 地址
     */
    const std::string& getUrl() const;

    /**
     * @brief 刷新
     */
    void refresh();

    /**
     * @brief 停止加载
     */
    void stopLoad();

    /**
     * @brief 后退
     */
    void goBack();

    /**
     * @brief 前进
     */
    void goForward();

    /**
     * @brief 能否后退
     * @return 返回 true 表示可以后退，否则为 false
     */
    bool canGoBack();

    /**
     * @brief 能否前进
     * @return 返回 true 表示可以前进，否则为 false
     */
    bool canGoForward();

    /**
     * @brief 是否加载中
     * @return 返回 true 表示加载中，否则为 false
     */
    bool isLoading();

    /**
     * @brief 开始一个下载任务
     * @param[in] url 要下载的文件地址
     */
    void startDownload(const std::string& url);

    /**
     * @brief 设置页面缩放比例
     * @param[in] zoom_level 比例值
     */
    void setZoomLevel(float zoom_level);

    /**
     * @brief 获取浏览器对象所属的窗体句柄
     * @return 窗口句柄
     */
    CefWindowHandle getWindowHandle() const;

    // /**
    // * @brief 注册一个 ProcessMessageHandler 对象，主要用来处理js消息
    // * @param [in] handler ProcessMessageHandler 对象指针
    // */
    // virtual void registerProcessMessageHandler(ProcessMessageHandler* handler);

    /**
     * @brief 打开开发者工具
     * @return 成功返回 true，失败返回 false
     */
    virtual bool openDevTools();

    /**
     * @brief 关闭开发者工具
     */
    virtual void closeDevTools();

    /**
     * @brief 判断是否打开开发者工具
     * @return 返回 true 表示已经绑定，false 为未绑定
     */
    virtual bool isDevToolsOpened() const { return _isDevToolsOpened; }

    /**
     * @brief 执行 JavaScript 代码
     * @param[in] script 一段可以执行的 JavaScript 代码
     */
    virtual void evaluateJavaScript(const std::string& script);

    /**
     * @brief 设置设备缩放因子
     * @param[in] device_scale_factor 设备缩放因子
     */
    void setDeviceScaleFactor(float device_scale_factor);
public:
#pragma region RenderHandler
    bool getRootScreenRect(CefRect& rect);
    bool getViewRect(CefRect& rect);
    bool getScreenPoint(int viewX, int viewY, int& screenX, int& screenY);
    bool getScreenInfo(CefScreenInfo& screenInfo);
    /**
     * @brief Handle CEF OnPaint callback for CPU-based software rendering
     */
    void onPaint(CefRenderHandler::PaintElementType type,
                 const CefRenderHandler::RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height);

    /**
     * @brief Handle CEF OnAcceleratedPaint callback for GPU hardware-accelerated rendering
     */
    void onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                            const CefRenderHandler::RectList& dirtyRects,
                            const CefAcceleratedPaintInfo& info);

    // void onPopupShow(bool show);

    // void onPopupSize(const CefRect &rect);

    bool startDragging(CefRefPtr<CefDragData> dragdata, CefRenderHandler::DragOperationsMask allowedops, int x, int y);

    void updateDragCursor(CefRenderHandler::DragOperation operation);

    void onImeCompositionRangeChanged(const CefRange& selectionRange, const CefRenderHandler::RectList& characterBounds);
#pragma endregion // RenderHandler

#pragma region CefDisplayHandler
     void onTitleChange(int browserId, const std::string& title);
     void onUrlChange(int browserId, const std::string& oldUrl, const std::string& url);
#pragma endregion // CefDisplayHandler

#pragma region LoadHandler
    void onLoadingStateChange(int browserId, bool isLoading, bool canGoBack, bool canGoForward);
    void onLoadStart(const std::string& url);
    void onLoadEnd(const std::string& url);
    void onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl);
#pragma endregion // LoadHandler

#pragma region LifeSpanHandler
    void onAfterCreated(int browserId);
    void onBeforeClose(int browserId);
#pragma endregion // LifeSpanHandler


    void onProcessMessageReceived(int browserId, const std::string& messageName, const std::string& jsonArgs);


#pragma region dragEvents
    // OsrDragEvents methods.
    CefBrowserHost::DragOperationsMask onDragEnter(CefRefPtr<CefDragData> dragData, CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
    CefBrowserHost::DragOperationsMask onDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
    void onDragLeave();
    CefBrowserHost::DragOperationsMask onDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
#pragma endregion // dragEvents

protected:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND createSubWindow(HWND parentHwnd, int x, int y,int width, int height, bool showWindow = true);

    void createCefBrowser(const std::string& url, const CefWebViewSetting& settings);

    void destroy();

    CefRefPtr<CefBrowser> getBrowser() const;

    // WndProc message handlers.
    void onMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
    void onSize();
    void onFocus(bool setFocus);
    void onCaptureLost();
    void onKeyEvent(UINT message, WPARAM wParam, LPARAM lParam);
    void onPaint();
    bool onTouchEvent(UINT message, WPARAM wParam, LPARAM lParam);

    void onIMESetContext(UINT message, WPARAM wParam, LPARAM lParam);
    void onIMEStartComposition();
    void onIMEComposition(UINT message, WPARAM wParam, LPARAM lParam);
    void onIMECancelCompositionEvent();
private:
    // Window Hwnd
    HWND _parentHwnd = nullptr;
    HWND _hwnd = nullptr;
    std::string _className;

    CefRefPtr<CefBrowser> _browser;
    CefRefPtr<CefViewClient> _client;
    std::shared_ptr<CefViewClientDelegate> _clientDelegate;

    std::unique_ptr<OsrRendererD3D11> _osrRenderer;

    typedef std::function<void(void)> StdClosure;
    std::vector<StdClosure> _taskListAfterCreated; // 浏览器创建后的任务队列
    bool _isDevToolsOpened = false;

    CefWebViewSetting _settings;
    RECT _clientRect;
    float _deviceScaleFactor = 0.f;

    // Mouse state tracking.
    POINT _lastMousePos;
    POINT _currentMousePos;
    bool _mouseRotation = false;
    bool _mouseTracking = false;
    int _lastClickX = 0;
    int _lastClickY = 0;
    CefBrowserHost::MouseButtonType _lastClickButton = MBT_LEFT;
    int _lastClickCount = 1;
    double _lastClickTime = 0;

    // drag
    std::shared_ptr<OsrDragEvents> _dragEvents;
    CComPtr<OsrDropTargetWin> _dropTarget;
    CefRenderHandler::DragOperation _currentDragOp;

    // ime
    // Class that encapsulates IMM32 APIs and controls IMEs attached to a window.
    std::unique_ptr<OsrImeHandlerWin> _imeHandler;
};
}

#endif //!CEFWEBVIEW_H