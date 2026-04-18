/**
 * @file        AppDelegate.mm
 * @brief       macOS application delegate implementation
 * @version     1.0
 * @author      heefuture
 * @date        2026.02.09
 * @copyright
 */
#import "AppDelegate.h"

#import "MainWindow.h"

#include "global/CefContext.h"

@implementation AppDelegate {
    MainWindow* _mainWindow;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _mainWindow = nil;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    // Create and show the main window
    _mainWindow = [[MainWindow alloc] init];
    [_mainWindow showWindow];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    // Return NO: window close does not trigger app termination.
    // Termination is driven by CefQuitMessageLoop after all browsers close.
    return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    // Close browser and quit CEF message loop
    if (_mainWindow) {
        [_mainWindow closeBrowser];
    }

    cefview::CefContext::instance().quitMessageLoop();

    return NSTerminateNow;
}

@end
