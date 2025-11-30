
/**
 * @file        CefWebView.h
 * @brief       Definition of CefWebView class
 *              Class for creating and managing CEF browser views
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

    // Disable copy and move operations
    CefWebView(const CefWebView&) = delete;
    CefWebView& operator=(const CefWebView&) = delete;
    CefWebView(CefWebView&&) = delete;
    CefWebView& operator=(CefWebView&&) = delete;

    void init();

    void closeBrowser();

    /**
     * @brief Set browser view bounds
     * @param[in] left New left position
     * @param[in] top New top position
     * @param[in] width New width
     * @param[in] height New height
     */
    void setBounds(int left, int top, int width, int height);

    void setVisible(bool bVisible = true);

    /**
     * @brief Load a URL
     * @param[in] url Website address to load
     */
    void loadUrl(const std::string& url);

    /**
     * @brief Get current page URL
     * @return Current URL address
     */
    const std::string& getUrl() const;

    /**
     * @brief Refresh the page
     */
    void refresh();

    /**
     * @brief Stop loading
     */
    void stopLoad();

    /**
     * @brief Navigate back
     */
    void goBack();

    /**
     * @brief Navigate forward
     */
    void goForward();

    /**
     * @brief Check if can navigate back
     * @return true if can go back, false otherwise
     */
    bool canGoBack();

    /**
     * @brief Check if can navigate forward
     * @return true if can go forward, false otherwise
     */
    bool canGoForward();

    /**
     * @brief Check if page is loading
     * @return true if loading, false otherwise
     */
    bool isLoading();

    /**
     * @brief Start a download task
     * @param[in] url URL of file to download
     */
    void startDownload(const std::string& url);

    /**
     * @brief Set page zoom level
     * @param[in] zoomLevel Zoom level value
     */
    void setZoomLevel(float zoomLevel);

    /**
     * @brief Get window handle of the browser
     * @return Window handle
     */
    CefWindowHandle getWindowHandle() const;

    /**
     * @brief Open developer tools
     * @return true if successful, false otherwise
     */
    virtual bool openDevTools();

    /**
     * @brief Close developer tools
     */
    virtual void closeDevTools();

    /**
     * @brief Check if developer tools is opened
     * @return true if opened, false otherwise
     */
    virtual bool isDevToolsOpened() const { return _isDevToolsOpened; }

    /**
     * @brief Execute JavaScript code
     * @param[in] script JavaScript code to execute
     */
    virtual void evaluateJavaScript(const std::string& script);

    /**
     * @brief Set device scale factor
     * @param[in] device_scale_factor Device scale factor value
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
    virtual void onPaint(CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList &dirtyRects,
                         const void *buffer,
                         int width,
                         int height);

    /**
     * @brief Handle CEF OnAcceleratedPaint callback for GPU hardware-accelerated rendering
     */
    virtual void onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                    const CefRenderHandler::RectList &dirtyRects,
                                    const CefAcceleratedPaintInfo &info);


    bool startDragging(CefRefPtr<CefDragData> dragdata, CefRenderHandler::DragOperationsMask allowedops, int x, int y);

    void updateDragCursor(CefRenderHandler::DragOperation operation);

    void onImeCompositionRangeChanged(const CefRange& selectionRange, const CefRenderHandler::RectList& characterBounds);
#pragma endregion // RenderHandler

#pragma region CefDisplayHandler
    bool onCursorChange(CefRefPtr<CefBrowser> browser,
                        CefCursorHandle cursor,
                        cef_cursor_type_t type,
                        const CefCursorInfo& customCursorInfo);
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

#pragma region dragEvents
    // OsrDragEvents methods.
    CefBrowserHost::DragOperationsMask onDragEnter(CefRefPtr<CefDragData> dragData, CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
    CefBrowserHost::DragOperationsMask onDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
    void onDragLeave();
    CefBrowserHost::DragOperationsMask onDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
#pragma endregion // dragEvents

    void onProcessMessageReceived(int browserId, const std::string& messageName, const std::string& jsonArgs);
protected:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND createSubWindow(HWND parentHwnd, int x, int y,int width, int height, bool showWindow = true);

    virtual void createCefBrowser(const std::string& url, const CefWebViewSetting& settings);

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
protected:
    // Window Hwnd
    HWND _parentHwnd = nullptr;
    HWND _hwnd = nullptr;
    std::string _className;

    CefRefPtr<CefBrowser> _browser;
    CefRefPtr<CefViewClient> _client;
    std::shared_ptr<CefViewClientDelegate> _clientDelegate;

    std::unique_ptr<OsrRendererD3D11> _osrRenderer;

    typedef std::function<void(void)> StdClosure;
    std::vector<StdClosure> _taskListAfterCreated; // Task queue to execute after browser is created
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