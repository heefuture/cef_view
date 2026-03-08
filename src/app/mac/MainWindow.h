/**
 * @file        MainWindow.h
 * @brief       macOS main window with vibrancy effect and CefWebView
 * @version     1.0
 * @author      heefuture
 * @date        2026.02.09
 * @copyright
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#pragma once

#import <Cocoa/Cocoa.h>

namespace cefview {
class CefContext;
}

@class CefWebView;

/// Main application window that hosts an NSVisualEffectView with a CefWebView overlay
@interface MainWindow : NSObject <NSWindowDelegate>

/// Initialize with a reference to the CefContext (must outlive this object)
- (instancetype)initWithCefContext:(cefview::CefContext*)context;

/// Show the window
- (void)showWindow;

/// Close the embedded browser
- (void)closeBrowser;

@end

#endif  // MAINWINDOW_H
