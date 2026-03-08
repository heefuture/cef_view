/**
 * @file        AppDelegate.h
 * @brief       macOS application delegate for CEF demo
 * @version     1.0
 * @author      heefuture
 * @date        2026.02.09
 * @copyright
 */
#ifndef APPDELEGATE_H
#define APPDELEGATE_H
#pragma once

#import <Cocoa/Cocoa.h>

namespace cefview {
class CefContext;
}

@class MainWindow;

/// NSApplicationDelegate that manages the application lifecycle and main window
@interface AppDelegate : NSObject <NSApplicationDelegate>

/// Initialize with a reference to the CefContext (must outlive this object)
- (instancetype)initWithCefContext:(cefview::CefContext*)context;

@end

#endif  // APPDELEGATE_H
