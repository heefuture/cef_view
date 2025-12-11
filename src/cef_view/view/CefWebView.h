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

#include <include/cef_base.h>
#include <include/cef_browser.h>
#include <include/cef_client.h>

namespace cefview {

class CefViewClientDelegate;
class CefViewClient;
class OsrRenderer;
class OsrDragEvents;
class OsrDropTargetWin;
class OsrImeHandlerWin;

/**
 * @brief CEF browser view class for creating and managing browser instances
 */
class CefWebView : public std::enable_shared_from_this<CefWebView> {
public:
    CefWebView(const CefWebViewSetting& settings);
    CefWebView(const CefWebViewSetting& settings, HWND parentHwnd);
    ~CefWebView();

    // Disable copy and move operations
    CefWebView(const CefWebView&) = delete;
    CefWebView& operator=(const CefWebView&) = delete;
    CefWebView(CefWebView&&) = delete;
    CefWebView& operator=(CefWebView&&) = delete;

    /**
     * @brief Initialize the web view with parent window
     * @param[in] parentHwnd Parent window handle
     */
    void init(HWND parentHwnd);

    /**
     * @brief Close the browser
     */
    void closeBrowser();

    /**
     * @brief Set browser view bounds
     * @param[in] left New left position
     * @param[in] top New top position
     * @param[in] width New width
     * @param[in] height New height
     */
    void setBounds(int left, int top, int width, int height);

    /**
     * @brief Set visibility of the browser view
     * @param[in] bVisible true to show, false to hide
     */
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
    virtual bool isDevToolsOpened() const;

    /**
     * @brief Execute JavaScript code
     * @param[in] script JavaScript code to execute
     */
    virtual void evaluateJavaScript(const std::string& script);

    /**
     * @brief Set device scale factor
     * @param[in] deviceScaleFactor Device scale factor value
     */
    virtual void setDeviceScaleFactor(float deviceScaleFactor);

public:
#pragma region RenderHandler
    /**
     * @brief Get the root screen rectangle
     * @param[out] rect Rectangle to receive root screen bounds
     * @return true if successful
     */
    bool getRootScreenRect(CefRect& rect);

    /**
     * @brief Get the view rectangle in screen coordinates
     * @param[out] rect Rectangle to receive view bounds
     * @return true if successful
     */
    virtual bool getViewRect(CefRect& rect);

    /**
     * @brief Convert view coordinates to screen coordinates
     * @param[in] viewX X coordinate in view space
     * @param[in] viewY Y coordinate in view space
     * @param[out] screenX X coordinate in screen space
     * @param[out] screenY Y coordinate in screen space
     * @return true if successful
     */
    bool getScreenPoint(int viewX, int viewY, int& screenX, int& screenY);

    /**
     * @brief Get screen information for the browser
     * @param[out] screenInfo Screen info structure to fill
     * @return true if successful
     */
    bool getScreenInfo(CefScreenInfo& screenInfo);

    /**
     * @brief Handle CEF OnPaint callback for CPU-based software rendering
     * @param[in] type Paint element type (view or popup)
     * @param[in] dirtyRects List of dirty rectangles to repaint
     * @param[in] buffer Pixel buffer data
     * @param[in] width Buffer width in pixels
     * @param[in] height Buffer height in pixels
     */
    virtual void onPaint(CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height);

    /**
     * @brief Handle CEF OnAcceleratedPaint callback for GPU hardware-accelerated rendering
     * @param[in] type Paint element type (view or popup)
     * @param[in] dirtyRects List of dirty rectangles to repaint
     * @param[in] info Accelerated paint information including shared texture handle
     */
    virtual void onAcceleratedPaint(CefRenderHandler::PaintElementType type,
                                    const CefRenderHandler::RectList& dirtyRects,
                                    const CefAcceleratedPaintInfo& info);

    /**
     * @brief Start a drag operation
     * @param[in] dragData Data being dragged
     * @param[in] allowedOps Allowed drag operations mask
     * @param[in] x Starting X coordinate
     * @param[in] y Starting Y coordinate
     * @return true if drag started successfully
     */
    bool startDragging(CefRefPtr<CefDragData> dragData,
                       CefRenderHandler::DragOperationsMask allowedOps,
                       int x,
                       int y);

    /**
     * @brief Update the drag cursor during drag operation
     * @param[in] operation Current drag operation type
     */
    void updateDragCursor(CefRenderHandler::DragOperation operation);

    /**
     * @brief Handle IME composition range change
     * @param[in] selectionRange Current selection range
     * @param[in] characterBounds Bounding rectangles for each character
     */
    void onImeCompositionRangeChanged(const CefRange& selectionRange, const CefRenderHandler::RectList& characterBounds);
#pragma endregion // RenderHandler

#pragma region CefDisplayHandler
    /**
     * @brief Handle cursor change event
     * @param[in] browser Browser instance
     * @param[in] cursor New cursor handle
     * @param[in] type Cursor type
     * @param[in] customCursorInfo Custom cursor information
     * @return true if handled
     */
    bool onCursorChange(CefRefPtr<CefBrowser> browser,
                        CefCursorHandle cursor,
                        cef_cursor_type_t type,
                        const CefCursorInfo& customCursorInfo);

    /**
     * @brief Handle page title change
     * @param[in] browserId Browser identifier
     * @param[in] title New page title
     */
    void onTitleChange(int browserId, const std::string& title);

    /**
     * @brief Handle URL change
     * @param[in] browserId Browser identifier
     * @param[in] oldUrl Previous URL
     * @param[in] url New URL
     */
    void onUrlChange(int browserId, const std::string& oldUrl, const std::string& url);
#pragma endregion // CefDisplayHandler

#pragma region LoadHandler
    /**
     * @brief Handle loading state change
     * @param[in] browserId Browser identifier
     * @param[in] isLoading Whether page is currently loading
     * @param[in] canGoBack Whether back navigation is available
     * @param[in] canGoForward Whether forward navigation is available
     */
    void onLoadingStateChange(int browserId, bool isLoading, bool canGoBack, bool canGoForward);

    /**
     * @brief Handle load start event
     * @param[in] url URL being loaded
     */
    void onLoadStart(const std::string& url);

    /**
     * @brief Handle load end event
     * @param[in] url URL that finished loading
     */
    void onLoadEnd(const std::string& url);

    /**
     * @brief Handle load error event
     * @param[in] browserId Browser identifier
     * @param[in] errorText Error description
     * @param[in] failedUrl URL that failed to load
     */
    void onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl);
#pragma endregion // LoadHandler

#pragma region LifeSpanHandler
    /**
     * @brief Handle browser creation completion
     * @param[in] browserId Browser identifier
     */
    void onAfterCreated(int browserId);

    /**
     * @brief Handle browser close event
     * @param[in] browserId Browser identifier
     */
    void onBeforeClose(int browserId);
#pragma endregion // LifeSpanHandler

#pragma region dragEvents
    /**
     * @brief Handle drag enter event
     * @param[in] dragData Data being dragged
     * @param[in] ev Mouse event information
     * @param[in] effect Allowed drag operations
     * @return Resulting drag operation mask
     */
    CefBrowserHost::DragOperationsMask onDragEnter(CefRefPtr<CefDragData> dragData, CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);

    /**
     * @brief Handle drag over event
     * @param[in] ev Mouse event information
     * @param[in] effect Allowed drag operations
     * @return Resulting drag operation mask
     */
    CefBrowserHost::DragOperationsMask onDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);

    /**
     * @brief Handle drag leave event
     */
    void onDragLeave();

    /**
     * @brief Handle drop event
     * @param[in] ev Mouse event information
     * @param[in] effect Allowed drag operations
     * @return Resulting drag operation mask
     */
    CefBrowserHost::DragOperationsMask onDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect);
#pragma endregion // dragEvents

    /**
     * @brief Handle process message received from renderer process
     * @param[in] browserId Browser identifier
     * @param[in] messageName Name of the message
     * @param[in] jsonArgs JSON formatted message arguments
     */
    void onProcessMessageReceived(int browserId, const std::string& messageName, const std::string& jsonArgs);
protected:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND createSubWindow(HWND parentHwnd, int x, int y, int width, int height, bool showWindow = true);

    virtual void createCefBrowser();

    void destroy();

    CefRefPtr<CefBrowser> getBrowser() const;

    // WndProc message handlers
    void onMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
    void onSize();
    void onFocus(bool setFocus);
    void onCaptureLost();
    virtual void onKeyEvent(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void onPaint();
    bool onTouchEvent(UINT message, WPARAM wParam, LPARAM lParam);

    // IME handlers
    void onIMESetContext(UINT message, WPARAM wParam, LPARAM lParam);
    void onIMEStartComposition();
    void onIMEComposition(UINT message, WPARAM wParam, LPARAM lParam);
    void onIMECancelCompositionEvent();

protected:
    // Window handles
    HWND _parentHwnd = nullptr;
    HWND _hwnd = nullptr;
    std::string _className;

    // CEF browser components
    CefRefPtr<CefBrowser> _browser;
    CefRefPtr<CefViewClient> _client;
    std::shared_ptr<CefViewClientDelegate> _clientDelegate;

    // OSR renderer
    std::unique_ptr<OsrRenderer> _osrRenderer;

    // Task queue to execute after browser is created
    using StdClosure = std::function<void(void)>;
    std::vector<StdClosure> _taskListAfterCreated;

    // Loading state and cached JS codes
    bool _isLoading = false;
    std::vector<std::string> _cachedJsCodes;

    // Settings and geometry
    CefWebViewSetting _settings;
    RECT _clientRect = {};
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

    // Drag and drop
    std::shared_ptr<OsrDragEvents> _dragEvents;
    CComPtr<OsrDropTargetWin> _dropTarget;
    CefRenderHandler::DragOperation _currentDragOp = DRAG_OPERATION_NONE;

    // IME handler
    std::unique_ptr<OsrImeHandlerWin> _imeHandler;
};

}  // namespace cefview

#endif  // CEFWEBVIEW_H
