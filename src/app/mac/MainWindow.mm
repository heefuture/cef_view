
/**
 * @file        MainWindow.mm
 * @brief       macOS main window implementation with vibrancy and CefWebView
 * @version     1.0
 * @author      heefuture
 * @date        2026.02.09
 * @copyright
 */
#import "MainWindow.h"

#import <Cocoa/Cocoa.h>

#include "view/CefWebViewSetting.h"

#import "CefWebView.h"

static const int kWindowWidth = 1280;
static const int kWindowHeight = 800;

@implementation MainWindow {
    NSWindow* _window;
    NSVisualEffectView* _effectView;
    CefWebView* _cefWebView;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _window = nil;
        _effectView = nil;
        _cefWebView = nil;
        [self createWindow];
    }
    return self;
}

- (void)createWindow {
    // Center the window on screen
    NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
    CGFloat x = NSMidX(screenFrame) - kWindowWidth / 2.0;
    CGFloat y = NSMidY(screenFrame) - kWindowHeight / 2.0;
    NSRect contentRect = NSMakeRect(x, y, kWindowWidth, kWindowHeight);

    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled
                                | NSWindowStyleMaskClosable
                                | NSWindowStyleMaskMiniaturizable
                                | NSWindowStyleMaskResizable;

    _window = [[NSWindow alloc] initWithContentRect:contentRect
                                          styleMask:styleMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"CEF View Demo Application"];
    [_window setDelegate:self];
    [_window setMinSize:NSMakeSize(640, 480)];

    // Setup vibrancy effect as content view
    NSRect viewFrame = [[_window contentView] bounds];

    _effectView = [[NSVisualEffectView alloc] initWithFrame:viewFrame];
    _effectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    _effectView.material = NSVisualEffectMaterialHUDWindow;
    _effectView.state = NSVisualEffectStateActive;
    _effectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [_window setContentView:_effectView];

    // Create CefWebView with transparent background
    [self createCefWebView];
}

- (void)createCefWebView {
    NSRect viewFrame = [_effectView bounds];

    cefview::CefWebViewSetting settings;
    settings.offScreenRenderingEnabled = true;
    settings.x = 0;
    settings.y = 0;
    settings.width = static_cast<int>(viewFrame.size.width);
    settings.height = static_cast<int>(viewFrame.size.height);
    settings.url = "https://www.bing.com";
    settings.transparentPaintingEnabled = true;
    settings.backgroundColor = 0x00000000;

    _cefWebView = [[CefWebView alloc] initWithFrame:viewFrame settings:settings];
    _cefWebView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Initialize the browser with the effect view as parent
    // Note: initWithParent: will add CefWebView to the parent view internally
    [_cefWebView initWithParent:_effectView];
}

- (void)showWindow {
    [_window makeKeyAndOrderFront:nil];
}

- (void)closeBrowser {
    if (_cefWebView) {
        [_cefWebView closeBrowser];
    }
}

#pragma mark - NSWindowDelegate

- (void)windowDidResize:(NSNotification*)notification {
    if (_cefWebView) {
        NSRect frame = [_effectView bounds];
        [_cefWebView setBoundsWithLeft:0
                                   top:0
                                 width:static_cast<int>(frame.size.width)
                                height:static_cast<int>(frame.size.height)];
    }
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    [self closeBrowser];
    [_window orderOut:nil];
    return NO;
}

- (void)windowWillClose:(NSNotification*)notification {
    _cefWebView = nil;
    _effectView = nil;
    _window = nil;
}

@end
