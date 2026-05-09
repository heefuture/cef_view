#ifndef CEFWEBVIEW_H
#define CEFWEBVIEW_H
#pragma once

#import <Cocoa/Cocoa.h>
#include <memory>
#include <string>

#include "include/cef_browser.h"
#include "view/CefWebViewSetting.h"

@class OsrCefTextInputClient;

namespace cefview {
class CefViewClient;
class CefViewClientDelegate;
class OsrRenderer;
}

/// macOS CefWebView - NSView subclass for CEF browser embedding.
/// Internal CEF callbacks are handled via the CefWebViewObserver protocol,
/// declared in CefViewClientDelegate.h and implemented in CefWebView.mm.
@interface CefWebView : NSView {
@protected
    float _deviceScaleFactor;
    CefRefPtr<CefBrowser> _browser;
    CefRefPtr<cefview::CefViewClient> _client;
    std::shared_ptr<cefview::CefViewClientDelegate> _clientDelegate;
    std::unique_ptr<cefview::OsrRenderer> _osrRenderer;
    cefview::CefWebViewSetting _settings;
    OsrCefTextInputClient* _textInputClient;
    NSTextInputContext* _textInputContextOsrMac;
    bool _editableFocused;
    bool _lazyInit;
}

/// Initialize with frame and settings. CefBrowser is created eagerly.
- (instancetype)initWithFrame:(NSRect)frame settings:(const cefview::CefWebViewSetting&)settings;

/// Initialize with frame, settings, and lazy init flag.
/// If lazyInit is YES, CefBrowser is NOT created during init; call initBrowser to create it later.
- (instancetype)initWithFrame:(NSRect)frame
                     settings:(const cefview::CefWebViewSetting&)settings
                     lazyInit:(BOOL)lazyInit;

/// Create the CefBrowser and load the URL from settings. No-op if not in lazy init state.
- (void)activate;

/// Close the browser and mark as lazy init state.
/// Call activate to recreate it later. No-op if already in lazy init state.
- (void)deactivate;

/// Whether the browser is active (not in deactivated/lazy state).
- (BOOL)isActive;

/// Callback invoked when browser is about to close (for coordinating window close)
@property (nonatomic, copy) void (^browserCloseCallback)(void);

/// Set visibility
- (void)setVisible:(BOOL)visible;

/// Get the view window handle
- (NSView*)getWindowHandle;

/// Get the CEF browser's internal window handle
- (void*)getBrowserWindowHandle;

/// Set the non-draggable area of the view.
/// Mouse down events outside this area will initiate a window drag operation.
/// Pass CGRectZero to disable window dragging.
/// @param area The non-draggable area in view coordinates (top-left origin, DIP units).
- (void)setNonDragArea:(CGRect)area;

/// Load a URL
- (void)loadUrl:(const std::string&)url;

/// Get cached URL
- (const std::string&)getUrl;

/// Get current URL from browser's main frame
- (std::string)getCurrentUrl;

/// Refresh the page
- (void)refresh;

/// Stop loading
- (void)stopLoad;

/// Navigate back
- (void)goBack;

/// Navigate forward
- (void)goForward;

/// Check if can navigate back
- (BOOL)canGoBack;

/// Check if can navigate forward
- (BOOL)canGoForward;

/// Check if page is loading
- (BOOL)isLoading;

/// Set page zoom level
- (void)setZoomLevel:(float)zoomLevel;

/// Set device scale factor
- (void)setDeviceScaleFactor:(float)deviceScaleFactor;

/// Open developer tools
- (BOOL)openDevTools;

/// Close developer tools
- (void)closeDevTools;

/// Check if developer tools is opened
- (BOOL)isDevToolsOpened;

/// Execute JavaScript code
- (void)evaluateJavaScript:(const std::string&)script;

/// Start a download task
- (void)startDownload:(const std::string&)url;

/// Notify screen info changed
- (void)notifyScreenInfoChanged;

/// Notify move or resize started
- (void)notifyMoveOrResizeStarted;

/// Check if focus is on editable field
- (BOOL)isEditableFocused;

/// Create the CEF browser instance. Subclasses can override to customize browser creation.
- (void)createCefBrowser;

/// Initialize the OSR renderer. Subclasses can override to use a custom renderer.
- (void)initOsrRenderer;

/// Factory hook for creating the OSR renderer. Subclasses override this to
/// swap in a platform- or product-specific renderer (e.g. Ardot's TGFX-based
/// compositor). The base implementation returns a Metal renderer and leaves
/// the Metal-to-OpenGL fallback to -initOsrRenderer.
/// @param width Initial width in DIP.
/// @param height Initial height in DIP.
/// @param transparent Whether transparent painting is requested.
/// @return A newly constructed renderer (not yet initialized), or nullptr on failure.
- (std::unique_ptr<cefview::OsrRenderer>)createOsrRendererWithWidth:(int)width
                                                             height:(int)height
                                                        transparent:(bool)transparent;

/// Create a CefKeyEvent from an NSEvent
- (CefKeyEvent)createCefKeyEventFromNSEvent:(NSEvent*)event;

/// Handle shortcut key events (Cmd+R, F12, etc.)
- (BOOL)handleShortcutKeyWithKeyCode:(int)keyCode modifiers:(uint32_t)modifiers;

/// Determine if a modifier key event represents key-up.
- (BOOL)isKeyUpEvent:(NSEvent*)event;

/// Handle focus change on editable field (triggered by renderer process message).
/// Subclasses can override to perform additional processing.
- (void)onEditableFocusChanged:(CefRefPtr<CefProcessMessage>)message;

/// Called when page load completes. Subclasses can override to perform additional processing.
- (void)onLoadEndWithUrl:(const std::string&)url;

@end

#endif  // CEFWEBVIEW_H
