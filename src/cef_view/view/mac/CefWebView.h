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
    bool _focusOnEditableField;
}

/// Initialize with frame and settings
- (instancetype)initWithFrame:(NSRect)frame settings:(const cefview::CefWebViewSetting&)settings;

/// Initialize with parent view (creates browser)
- (void)initWithParent:(NSView*)parentView;

/// Close the browser
- (void)closeBrowser;

/// Callback invoked when browser is about to close (for coordinating window close)
@property (nonatomic, copy) void (^browserCloseCallback)(void);

/// Set browser view bounds
- (void)setBoundsWithLeft:(int)left top:(int)top width:(int)width height:(int)height;

/// Set visibility
- (void)setViewVisible:(BOOL)visible;

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
- (BOOL)isFocusOnEditableField;

/// Create the CEF browser instance. Subclasses can override to customize browser creation.
- (void)createCefBrowser;

/// Initialize the OSR renderer. Subclasses can override to use a custom renderer.
- (void)initOsrRenderer;

/// Create a CefKeyEvent from an NSEvent
- (CefKeyEvent)createCefKeyEventFromNSEvent:(NSEvent*)event;

/// Handle shortcut key events (Cmd+R, F12, etc.)
- (BOOL)handleShortcutKeyWithKeyCode:(int)keyCode modifiers:(uint32_t)modifiers;

@end

#endif  // CEFWEBVIEW_H
