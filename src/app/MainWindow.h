
/**
* @file        mainWindow.h
* @brief
* @version     1.0
* @author      heefuture
* @date        2025.07.31
* @copyright
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include <list>

class CefWebView;
// Windows implementation of a top-level native window in the browser process.
// The methods of this class must be called on the main thread unless otherwise
// indicated.
class MainWindow
{
public:
    enum class ShowMode {
        NORMAL,
        // Show the window as minimized.
        MINIMIZED,
        // Show the window as maximized.
        MAXIMIZED,
        // Show the window as fullscreen.
        FULLSCREEN,
        // Show the window without activating it.
        NO_ACTIVATE,
        // Show the window as hidden (no dock thumbnail).
        // Only supported on MacOS.
        // CEF_SHOW_STATE_HIDDEN,
    };

    // Used to configure how a MainWindow is created.
    struct Config
    {
        // Configure the show mode.
        ShowMode showMode = ShowMode::NORMAL;
        // If true the window will always display above other windows.
        bool alwaysOnTop = false;
        // If true the window will be created initially hidden.
        bool initiallyHidden = false;
        bool noActivate = false;
        // Requested window position. If |bounds| and |source_bounds| are empty the
        // default window size and location will be used.
        RECT bounds;
        // Position of the UI element that triggered the window creation. If |bounds|
        // is empty and |source_bounds| is non-empty the new window will be positioned
        // relative to |sourceBounds|. This is currently only implemented for Views-
        // hosted windows when |initiallyHidden| is also true.
        RECT sourceBounds;
    };

    // Constructor may be called on any thread.
    explicit MainWindow();
    ~MainWindow();

    // RootWindow methods.
    void init(std::unique_ptr<Config> config);
    void show(ShowMode mode);
    void hide();
    void setBounds(int x, int y, size_t width, size_t height);
    void close(bool force);
    void setDeviceScaleFactor(float deviceScaleFactor);
    float getDeviceScaleFactor() const;

    HWND getWindowHandle() const;

    // CefRefPtr<CefBrowser> getBrowser() const;
private:
    // void createBrowserWindow(const std::string& startup_url);
    void createRootWindow(bool initiallyHidden);

    // Register the root window class.
    static void RegisterRootClass(HINSTANCE hInstance,
                                  const std::wstring &windowClass,
                                  HBRUSH backgroundBrush);

    // Window procedure for the edit field.
    static LRESULT CALLBACK EditWndProc(HWND hWnd,
                                        UINT message,
                                        WPARAM wParam,
                                        LPARAM lParam);

    // Window procedure for the find dialog.
    static LRESULT CALLBACK FindWndProc(HWND hWnd,
                                        UINT message,
                                        WPARAM wParam,
                                        LPARAM lParam);

    // Window procedure for the root window.
    static LRESULT CALLBACK RootWndProc(HWND hWnd,
                                        UINT message,
                                        WPARAM wParam,
                                        LPARAM lParam);

    // Event handlers.
    void onPaint();
    void onFocus();
    void onActivate(bool active);
    void onSize(bool minimized);
    void onMove();
    void onDpiChanged(WPARAM wParam, LPARAM lParam);
    bool onEraseBkgnd();
    bool onCommand(UINT id);
    void onFind();
    void onFindEvent();
    void onAbout();
    void onNCCreate(LPCREATESTRUCT lpCreateStruct);
    void onCreate(LPCREATESTRUCT lpCreateStruct);
    bool onClose();
    void onDestroyed();

    void NotifyDestroyedIfDone();

    static void SaveWindowRestoreOnUIThread(const WINDOWPLACEMENT &placement);

private:
    bool _initialized = false;
    bool _alwaysOnTop = false;
    float _deviceScaleFactor = 1.0f;
    RECT _initialBounds;
    ShowMode _initialShowMode = ShowMode::NORMAL;
    std::list<std::shared_ptr<CefWebView>> _cefViewList;

    // Main window.
    HWND _hwnd = nullptr;

    // Draggable region.
    HRGN _draggableRegion = nullptr;

    bool _windowDestroyed = false;

    bool _calledEnableNonClientDpiScaling = false;
};

#endif //!MAINWINDOW_H
