
/**
 * @file        MainWindow.h
 * @brief       Main application window implementation
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
#include <vector>

#include <utils/WinMaterial.h>

namespace cefview {
class CefWebView;
class WinCompositionBackdrop;
}

using cefview::CefWebView;

/**
 * @brief Windows implementation of a top-level native window in the browser process
 *
 * The methods of this class must be called on the main thread unless otherwise indicated.
 */
class MainWindow
{
public:
    /**
     * @brief Window show mode enumeration
     */
    enum class ShowMode {
        NORMAL,
        MINIMIZED,      ///< Show the window as minimized
        MAXIMIZED,      ///< Show the window as maximized
        FULLSCREEN,     ///< Show the window as fullscreen
        NO_ACTIVATE,    ///< Show the window without activating it
    };

    /**
     * @brief Configuration structure for MainWindow creation
     */
    struct Config
    {
        ShowMode showMode = ShowMode::NORMAL;   ///< Configure the show mode
        bool alwaysOnTop = false;               ///< Window will always display above other windows
        bool initiallyHidden = false;           ///< Window will be created initially hidden
        bool noActivate = false;
        RECT bounds;                            ///< Requested window position and size
        RECT sourceBounds;                      ///< Position of UI element that triggered window creation
    };

    /**
     * @brief Constructor (may be called on any thread)
     */
    explicit MainWindow();
    ~MainWindow();

    /**
     * @brief Initialize the main window with configuration
     * @param config Window configuration
     */
    void init(std::unique_ptr<Config> config);

    /**
     * @brief Show the window with specified mode
     * @param mode Show mode
     */
    void show(ShowMode mode);

    /**
     * @brief Hide the window
     */
    void hide();

    /**
     * @brief Set window bounds
     * @param x X position
     * @param y Y position
     * @param width Window width
     * @param height Window height
     */
    void setBounds(int x, int y, size_t width, size_t height);

    /**
     * @brief Close the window
     * @param force Force close without prompting
     */
    void close(bool force);

    /**
     * @brief Set device scale factor for DPI scaling
     * @param deviceScaleFactor Scale factor
     */
    void setDeviceScaleFactor(float deviceScaleFactor);

    /**
     * @brief Get current device scale factor
     * @return Scale factor
     */
    float getDeviceScaleFactor() const;

    /**
     * @brief Get window handle
     * @return HWND handle
     */
    HWND getWindowHandle() const;

    /**
     * @brief Create top view browser
     */
    void createTopView();

    /**
     * @brief Create bottom view browsers
     */
    void createBottomViews();

    /**
     * @brief Set active bottom view
     * @param index View index
     */
    void setActiveBottomView(int index);

    /**
     * @brief Update window layout
     */
    void updateLayout();

private:
    void createRootWindow(bool initiallyHidden);
    void createCefViews();

    /**
     * @brief Register the root window class
     */
    static void RegisterRootClass(HINSTANCE hInstance,
                                  const std::wstring &windowClass,
                                  HBRUSH backgroundBrush);

    /**
     * @brief Window procedure for the root window
     */
    static LRESULT CALLBACK RootWndProc(HWND hWnd,
                                        UINT message,
                                        WPARAM wParam,
                                        LPARAM lParam);

    // Event handlers
    void onPaint();
    void onFocus();
    void onActivate(bool active);
    void onSize(bool minimized);
    void onMove();
    void onDpiChanged(WPARAM wParam, LPARAM lParam);
    bool onCommand(UINT id);
    void onAbout();
    void onNCCreate(LPCREATESTRUCT lpCreateStruct);
    void onCreate(LPCREATESTRUCT lpCreateStruct);
    bool onClose();
    void onDestroyed();

private:
    bool _initialized = false;
    bool _alwaysOnTop = false;
    float _deviceScaleFactor = 1.0f;
    RECT _initialBounds = {};
    ShowMode _initialShowMode = ShowMode::NORMAL;

    std::shared_ptr<CefWebView> _topView;                        ///< Top view (independent browser)
    std::vector<std::shared_ptr<CefWebView>> _bottomViews;       ///< Bottom view list (overlay display)
    int _activeBottomViewIndex = 0;                              ///< Currently active bottom view index

    HWND _btnSwitch1 = nullptr;                                  ///< Switch button 1
    HWND _btnSwitch2 = nullptr;                                  ///< Switch button 2
    HWND _btnSwitch3 = nullptr;                                  ///< Switch button 3

    std::unique_ptr<cefview::WinCompositionBackdrop> _backdrop;  ///< Composition backdrop effect

    HWND _hwnd = nullptr;                                        ///< Main window handle
    HRGN _draggableRegion = nullptr;                             ///< Draggable region

    bool _windowDestroyed = false;
    bool _windowCreated = false;
    bool _calledEnableNonClientDpiScaling = false;

    // GDI+ initialization token
    ULONG_PTR _gdiplusToken = 0;
};

#endif //!MAINWINDOW_H
