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
    cefview::CefContext* _cefContext;
    MainWindow* _mainWindow;
}

- (instancetype)initWithCefContext:(cefview::CefContext*)context {
    self = [super init];
    if (self) {
        _cefContext = context;
        _mainWindow = nil;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    // Create and show the main window
    _mainWindow = [[MainWindow alloc] initWithCefContext:_cefContext];
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

    if (_cefContext) {
        _cefContext->quitMessageLoop();
    }

    return NSTerminateNow;
}

@end
