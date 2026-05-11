
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

#include <string>

#include "view/CefWebViewSetting.h"
#include "utils/PathUtil.h"

#import "CefWebView.h"

static const int kWindowWidth = 1280;
static const int kWindowHeight = 800;

@interface MainWindow ()

- (void)createWindow;
- (void)createTopView;
- (void)createBottomTabs;
- (void)updateLayout;

@end

@implementation MainWindow {
    NSWindow* _window;
    NSVisualEffectView* _effectView;
    CefWebView* _topView;
    NSTabView* _bottomTabView;
    NSMutableArray<CefWebView*>* _bottomViews;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _window = nil;
        _effectView = nil;
        _topView = nil;
        _bottomTabView = nil;
        _bottomViews = [[NSMutableArray alloc] init];
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

    // Build the split layout: top CefWebView + bottom NSTabView of CefWebViews.
    [self createTopView];
    [self createBottomTabs];
    [self updateLayout];
}

- (void)createTopView {
    NSRect effectBounds = [_effectView bounds];
    CGFloat topHeight = effectBounds.size.height / 2.0;
    CGFloat bottomHeight = effectBounds.size.height - topHeight;

    // Top view sits above the NSTabView in flipped-off coordinates.
    NSRect topFrame = NSMakeRect(0, bottomHeight, effectBounds.size.width, topHeight);

    std::string resourcePath = cefview::PathUtil::GetResourcePath("index.html");
    // macOS absolute paths start with '/', so use the "file://" + path form
    // to avoid producing the malformed "file:////" prefix.
    std::string url = "file://" + resourcePath;

    cefview::CefWebViewSetting settings;
    settings.offScreenRenderingEnabled = true;
    settings.x = 0;
    settings.y = 0;
    settings.width = static_cast<int>(topFrame.size.width);
    settings.height = static_cast<int>(topFrame.size.height);
    settings.url = url;
    settings.transparentPaintingEnabled = true;
    settings.backgroundColor = 0x00000000;

    _topView = [[CefWebView alloc] initWithFrame:topFrame settings:settings];
    _topView.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;

    [_effectView addSubview:_topView];
}

- (void)createBottomTabs {
    NSRect effectBounds = [_effectView bounds];
    CGFloat topHeight = effectBounds.size.height / 2.0;
    CGFloat bottomHeight = effectBounds.size.height - topHeight;

    NSRect bottomFrame = NSMakeRect(0, 0, effectBounds.size.width, bottomHeight);
    _bottomTabView = [[NSTabView alloc] initWithFrame:bottomFrame];
    _bottomTabView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Attach the tab view first so -contentRect reports a real frame before
    // we size the per-tab containers.
    [_effectView addSubview:_bottomTabView];

    NSRect tabContentRect = [_bottomTabView contentRect];

    NSString* const kLabels[] = { @"Bing", @"Google", @"GitHub" };
    const char* const kUrls[] = {
        "https://www.bing.com",
        "https://www.google.com",
        "https://github.com"
    };
    const NSInteger kTabCount = 3;

    for (NSInteger i = 0; i < kTabCount; ++i) {
        NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier:@(i)];
        item.label = kLabels[i];

        NSRect containerFrame = NSMakeRect(0, 0, tabContentRect.size.width, tabContentRect.size.height);
        NSView* container = [[NSView alloc] initWithFrame:containerFrame];
        container.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        item.view = container;

        cefview::CefWebViewSetting settings;
        // Bottom tabs host remote web pages and do not need to composite
        // with the host window's vibrancy effect, so they run in native
        // window mode for pixel-perfect rendering (same path as Chrome
        // and cefclient's --use-views). The top view stays OSR because
        // it draws over the translucent NSVisualEffectView.
        settings.offScreenRenderingEnabled = false;
        settings.x = 0;
        settings.y = 0;
        settings.width = static_cast<int>(containerFrame.size.width);
        settings.height = static_cast<int>(containerFrame.size.height);
        settings.url = kUrls[i];
        settings.transparentPaintingEnabled = false;
        settings.backgroundColor = 0xffffffff;

        CefWebView* web = [[CefWebView alloc] initWithFrame:containerFrame settings:settings];
        web.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

        [container addSubview:web];
        [_bottomTabView addTabViewItem:item];
        [_bottomViews addObject:web];
    }
}

- (void)updateLayout {
    if (!_effectView) return;

    NSRect effectBounds = [_effectView bounds];
    CGFloat width = effectBounds.size.width;
    CGFloat topHeight = effectBounds.size.height / 2.0;
    CGFloat bottomHeight = effectBounds.size.height - topHeight;

    // Explicitly size the two top-level siblings. Children (CefWebView
    // instances inside NSTabViewItem containers) follow via
    // autoresizingMask; CefWebView syncs its OSR renderer / browser from
    // -setFrameSize: internally, so no manual per-view fix-up is needed.
    if (_topView) {
        _topView.frame = NSMakeRect(0, bottomHeight, width, topHeight);
    }

    if (_bottomTabView) {
        _bottomTabView.frame = NSMakeRect(0, 0, width, bottomHeight);
    }
}

- (void)showWindow {
    [_window makeKeyAndOrderFront:nil];
}

- (void)closeBrowser {
    if (_topView) {
        [_topView deactivate];
    }
    for (CefWebView* web in _bottomViews) {
        [web deactivate];
    }
}

#pragma mark - NSWindowDelegate

- (void)windowDidResize:(NSNotification*)notification {
    [self updateLayout];
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    [self closeBrowser];
    [_window orderOut:nil];
    return NO;
}

- (void)windowWillClose:(NSNotification*)notification {
    _topView = nil;
    [_bottomViews removeAllObjects];
    _bottomViews = nil;
    _bottomTabView = nil;
    _effectView = nil;
    _window = nil;
}

@end
