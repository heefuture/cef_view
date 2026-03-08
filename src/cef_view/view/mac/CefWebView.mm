#import "CefWebView.h"
#import "CefViewClientDelegate.h"
#include "client/CefViewClient.h"
#include "osr/OsrRenderer.h"
#include "osr/OsrRendererGL.h"
#include "osr/mac/OsrRendererMetal.h"
#import "OsrCefTextInputClient.h"
#include "utils/LogUtil.h"
#include "utils/ScreenUtil.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/base/cef_callback.h"

#include "osr/BytesWriteHandler.h"
#include "include/cef_parser.h"

#include <sstream>
#include <functional>

using namespace cefview;

static NSString* const kCEFDragDummyPboardType = @"org.CEF.drag-dummy-type";
static NSString* const kNSURLTitlePboardType = @"public.url-name";

// Private extension - conforms to CefWebViewObserver for internal CEF callbacks
@interface CefWebView () <NSDraggingSource, NSDraggingDestination, CefWebViewObserver>
@end

@implementation CefWebView {
    // Mouse tracking
    NSTrackingArea* _trackingArea;

    // Drag and drop
    CefRefPtr<CefDragData> _currentDragData;
    NSDragOperation _currentDragOp;
    NSDragOperation _currentAllowedOps;
    NSPasteboard* _pasteboard;
    NSString* _fileUTI;

    // IME
    NSTextInputContext* _textInputContextOsrMac;

    // Event monitors
    id _keyEventMonitor;
    id _endWheelMonitor;

    // Window drag
    bool _movingWindow;
    CGRect _nonDragArea;

    // Task queue
    std::vector<std::function<void(void)>> _taskListAfterCreated;

    // Loading state
    bool _isLoading;
    std::vector<std::string> _cachedJsCodes;

    // DevTools window
    NSWindow* _devToolsWindow;
}

#pragma mark - Initialization

- (instancetype)initWithFrame:(NSRect)frame settings:(const CefWebViewSetting&)settings
{
    self = [super initWithFrame:frame];
    if (self) {
        _settings = settings;
        _deviceScaleFactor = 1.0f;
        _isLoading = false;
        _focusOnEditableField = false;
        _currentDragOp = NSDragOperationNone;
        _currentAllowedOps = NSDragOperationNone;
        _pasteboard = nil;
        _fileUTI = nil;
        _endWheelMonitor = nil;
        _movingWindow = false;
        _nonDragArea = CGRectZero;
        _devToolsWindow = nil;

        // Setup tracking area for mouse events
        [self setupTrackingArea];

        // Register for drag and drop
        [self registerForDraggedTypes:@[kCEFDragDummyPboardType,
                                        NSPasteboardTypeFileURL,
                                        NSPasteboardTypeString]];
    }
    return self;
}

- (void)initWithParent:(NSView*)parentView
{
    if (!parentView) return;

    // Get device scale factor from window
    NSWindow* window = [parentView window];
    if (window) {
        _deviceScaleFactor = [window backingScaleFactor];
    }

    // Update settings if size is not specified
    if (_settings.width <= 0 || _settings.height <= 0) {
        NSRect parentBounds = [parentView bounds];
        _settings.x = 0;
        _settings.y = 0;
        _settings.width = static_cast<int>(parentBounds.size.width);
        _settings.height = static_cast<int>(parentBounds.size.height);
    }

    // Add to parent view
    [parentView addSubview:self];

    // Create CEF browser
    [self createCefBrowser];
}

- (void)dealloc
{
    [self closeBrowser];
    [self removeKeyEventMonitor];

    if (_endWheelMonitor) {
        [NSEvent removeMonitor:_endWheelMonitor];
        _endWheelMonitor = nil;
    }

    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }

    _clientDelegate.reset();
    _osrRenderer.reset();
    _taskListAfterCreated.clear();
    _cachedJsCodes.clear();
}

- (void)closeBrowser
{
    if (_client && !_client->IsClosing()) {
        _client->CloseAllBrowser();
    }
}

#pragma mark - Browser Creation

- (void)createCefBrowser
{
    if (_clientDelegate) return;

    _clientDelegate = std::make_shared<CefViewClientDelegate>(self);
    _client = new CefViewClient(_clientDelegate);

    CefWindowInfo windowInfo;

    if (_settings.offScreenRenderingEnabled) {
        // Off-screen rendering mode
        windowInfo.SetAsWindowless((__bridge void*)self);
        windowInfo.shared_texture_enabled = true;
    } else {
        // Native window mode
        NSRect bounds = NSMakeRect(0, 0, _settings.width, _settings.height);
        windowInfo.SetAsChild((__bridge void*)self, CefRect(0, 0, _settings.width, _settings.height));
    }
    windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

    CefBrowserSettings browserSettings;
    browserSettings.background_color = _settings.backgroundColor;
    browserSettings.windowless_frame_rate = _settings.windowlessFrameRate;

    CefBrowserHost::CreateBrowser(windowInfo, _client, CefString(_settings.url), browserSettings, nullptr, nullptr);

    if (_settings.offScreenRenderingEnabled) {
        [self initOsrRenderer];
    }
}

- (void)initOsrRenderer
{
    NSRect frame = [self bounds];
    int width = static_cast<int>(frame.size.width);
    int height = static_cast<int>(frame.size.height);
    bool transparent = _settings.transparentPaintingEnabled;

    // Prefer Metal renderer on macOS
    _osrRenderer = std::make_unique<OsrRendererMetal>(
        (__bridge void*)self, width, height, transparent);

    if (!_osrRenderer->initialize()) {
        LOGW << "Metal renderer init failed, falling back to OpenGL";
        _osrRenderer = std::make_unique<OsrRendererGL>(
            (__bridge void*)self, width, height, transparent);
        if (!_osrRenderer->initialize()) {
            LOGE << "OpenGL renderer init also failed";
            _osrRenderer.reset();
            return;
        }
    }

    _osrRenderer->setDeviceScaleFactor(_deviceScaleFactor);
}

#pragma mark - Tracking Area

- (void)setupTrackingArea
{
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
    }

    NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                   NSTrackingMouseMoved |
                                   NSTrackingActiveInKeyWindow |
                                   NSTrackingInVisibleRect;

    _trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                 options:options
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    [self setupTrackingArea];
}

#pragma mark - Key Event Monitor

- (void)setupKeyEventMonitor
{
    if (_keyEventMonitor) return;

    __weak CefWebView* weakSelf = self;
    _keyEventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                             handler:^NSEvent*(NSEvent* event) {
        CefWebView* strongSelf = weakSelf;
        if (!strongSelf) return event;

        // Handle Command+. (stop key)
        if (([event modifierFlags] & NSEventModifierFlagCommand) &&
            [[event characters] isEqualToString:@"."]) {
            if (strongSelf->_browser) {
                CefKeyEvent keyEvent = [strongSelf createCefKeyEventFromNSEvent:event];
                keyEvent.type = KEYEVENT_KEYDOWN;
                strongSelf->_browser->GetHost()->SendKeyEvent(keyEvent);
            }
            return nil;
        }
        return event;
    }];
}

- (void)removeKeyEventMonitor
{
    if (_keyEventMonitor) {
        [NSEvent removeMonitor:_keyEventMonitor];
        _keyEventMonitor = nil;
    }
}

#pragma mark - Window Management

- (void)setBoundsWithLeft:(int)left top:(int)top width:(int)width height:(int)height
{
    if (width <= 0 || height <= 0) return;

    bool sizeChanged = (width != _settings.width || height != _settings.height);

    _settings.x = left;
    _settings.y = top;
    _settings.width = width;
    _settings.height = height;

    [self setFrame:NSMakeRect(left, top, width, height)];

    if (_settings.offScreenRenderingEnabled && _osrRenderer) {
        _osrRenderer->setBounds(0, 0, width, height);
    }

    if (_browser) {
        _browser->GetHost()->WasResized();
        if (sizeChanged) {
            _browser->GetHost()->Invalidate(PET_VIEW);
        }
    }

    if (sizeChanged) {
        [self setNeedsDisplay:YES];
    }
}

- (void)setNonDragArea:(CGRect)area {
    _nonDragArea = area;
}

- (void)setViewVisible:(BOOL)visible
{
    [self setHidden:!visible];

    if (_browser) {
        _browser->GetHost()->WasHidden(!visible);
    }
}

- (NSView*)getWindowHandle
{
    return self;
}

- (void*)getBrowserWindowHandle
{
    if (_browser) {
        return _browser->GetHost()->GetWindowHandle();
    }
    return nullptr;
}

#pragma mark - URL and Navigation

- (void)loadUrl:(const std::string&)url
{
    if (_browser) {
        CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
        if (frame) {
            frame->LoadURL(url);
            _settings.url = url;
        }
    } else {
        std::function<void(void)> loadUrlTask = [self, url]() {
            if (_browser) {
                CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
                if (frame) {
                    frame->LoadURL(url);
                    _settings.url = url;
                }
            }
        };
        _taskListAfterCreated.push_back(loadUrlTask);
    }
}

- (const std::string&)getUrl
{
    return _settings.url;
}

- (std::string)getCurrentUrl
{
    if (!_browser) {
        return _settings.url;
    }
    CefRefPtr<CefFrame> mainFrame = _browser->GetMainFrame();
    if (!mainFrame) {
        return _settings.url;
    }
    return mainFrame->GetURL().ToString();
}

- (void)refresh
{
    if (_browser) {
        _browser->Reload();
    }
}

- (void)stopLoad
{
    if (_browser) {
        _browser->StopLoad();
    }
}

- (void)goBack
{
    if (_browser) {
        _browser->GoBack();
    }
}

- (void)goForward
{
    if (_browser) {
        _browser->GoForward();
    }
}

- (BOOL)canGoBack
{
    if (_browser) {
        return _browser->CanGoBack();
    }
    return NO;
}

- (BOOL)canGoForward
{
    if (_browser) {
        return _browser->CanGoForward();
    }
    return NO;
}

- (BOOL)isLoading
{
    if (_browser) {
        return _browser->IsLoading();
    }
    return NO;
}

#pragma mark - Zoom and Rendering

- (void)setZoomLevel:(float)zoomLevel
{
    if (_browser) {
        _browser->GetHost()->SetZoomLevel(zoomLevel);
    }
}

- (void)setDeviceScaleFactor:(float)deviceScaleFactor
{
    if (_deviceScaleFactor == deviceScaleFactor) return;

    _deviceScaleFactor = deviceScaleFactor;

    if (_browser) {
        _browser->GetHost()->NotifyScreenInfoChanged();
        _browser->GetHost()->WasResized();
        _browser->GetHost()->Invalidate(PET_VIEW);
    }

    if (_osrRenderer) {
        _osrRenderer->setDeviceScaleFactor(deviceScaleFactor);
    }
}

#pragma mark - Developer Tools

- (BOOL)openDevTools
{
    if (!_browser) return NO;

    if (_browser->GetHost()->HasDevTools()) {
        if (_devToolsWindow && [_devToolsWindow isVisible]) {
            [_devToolsWindow makeKeyAndOrderFront:nil];
        }
        return YES;
    }

    // Create DevTools window
    NSRect frame = NSMakeRect(100, 100, 1200, 800);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

    _devToolsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                  styleMask:styleMask
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
    [_devToolsWindow setTitle:@"DevTools"];
    [_devToolsWindow setReleasedWhenClosed:NO];

    NSView* contentView = [_devToolsWindow contentView];
    NSRect contentBounds = [contentView bounds];

    CefWindowInfo windowInfo;
    windowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    CefRect cefRect(0, 0, static_cast<int>(contentBounds.size.width),
                    static_cast<int>(contentBounds.size.height));
    windowInfo.SetAsChild((__bridge void*)contentView, cefRect);

    CefBrowserSettings settings;
    _browser->GetHost()->ShowDevTools(windowInfo, nullptr, settings, CefPoint());

    [_devToolsWindow makeKeyAndOrderFront:nil];
    return YES;
}

- (void)closeDevTools
{
    if (_browser && _browser->GetHost()->HasDevTools()) {
        _browser->GetHost()->CloseDevTools();
    }
    if (_devToolsWindow) {
        [_devToolsWindow close];
        _devToolsWindow = nil;
    }
}

- (BOOL)isDevToolsOpened
{
    if (_browser) {
        return _browser->GetHost()->HasDevTools();
    }
    return NO;
}

#pragma mark - JavaScript Execution

- (void)evaluateJavaScript:(const std::string&)script
{
    if (script.empty()) return;

    if (_isLoading || !_browser) {
        _cachedJsCodes.push_back(script);
        return;
    }

    CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
    if (frame) {
        frame->ExecuteJavaScript(CefString(script), frame->GetURL(), 0);
    }
}

#pragma mark - Download

- (void)startDownload:(const std::string&)url
{
    if (_browser) {
        _browser->GetHost()->StartDownload(url);
    }
}

#pragma mark - Screen and Rendering Info (OSR)

- (BOOL)getRootScreenRect:(CefRect&)rect
{
    NSWindow* window = [self window];
    if (!window) return NO;

    NSRect windowFrame = [window frame];
    NSScreen* screen = [window screen];
    if (!screen) screen = [NSScreen mainScreen];

    NSRect screenFrame = [screen frame];
    rect.x = static_cast<int>(windowFrame.origin.x);
    rect.y = static_cast<int>(screenFrame.size.height - windowFrame.origin.y - windowFrame.size.height);
    rect.width = static_cast<int>(windowFrame.size.width);
    rect.height = static_cast<int>(windowFrame.size.height);
    return YES;
}

- (BOOL)getViewRect:(CefRect&)rect
{
    rect.x = 0;
    rect.y = 0;

    // |bounds| is in OS X view coordinates.
    NSRect bounds = [self bounds];

    // Convert to device coordinates.
    bounds = [self convertRectToBacking:bounds];

    // Convert to DIP coordinates.
    rect.width = ScreenUtil::DeviceToLogical(static_cast<int>(bounds.size.width), _deviceScaleFactor);
    if (rect.width == 0)
        rect.width = 1;
    rect.height = ScreenUtil::DeviceToLogical(static_cast<int>(bounds.size.height), _deviceScaleFactor);
    if (rect.height == 0)
        rect.height = 1;

    return YES;
}

- (BOOL)getScreenPointWithViewX:(int)viewX viewY:(int)viewY screenX:(int*)screenX screenY:(int*)screenY
{
    NSWindow* window = [self window];
    if (!window) return NO;

    // (viewX, viewY) is in browser DIP coordinates.
    // Convert to device coordinates.
    NSPoint viewPoint = NSMakePoint(
        ScreenUtil::LogicalToDevice(viewX, _deviceScaleFactor),
        ScreenUtil::LogicalToDevice(viewY, _deviceScaleFactor));

    // Convert to OS X view coordinates.
    viewPoint = [self convertPointFromBacking:viewPoint];

    // Reverse the Y component.
    NSRect bounds = [self bounds];
    viewPoint.y = bounds.size.height - viewPoint.y;

    // Convert to screen coordinates.
    NSPoint windowPoint = [self convertPoint:viewPoint toView:nil];
    NSPoint screenPoint = [window convertPointToScreen:windowPoint];

    *screenX = static_cast<int>(screenPoint.x);
    *screenY = static_cast<int>(screenPoint.y);

    return YES;
}

- (BOOL)getScreenInfo:(CefScreenInfo&)screenInfo
{
    NSWindow* window = [self window];
    if (!window) return NO;

    screenInfo.device_scale_factor = _deviceScaleFactor;

    CefRect viewRect;
    [self getViewRect:viewRect];
    screenInfo.rect = viewRect;
    screenInfo.available_rect = viewRect;

    return YES;
}

- (void)notifyScreenInfoChanged
{
    if (_browser) {
        _browser->GetHost()->NotifyScreenInfoChanged();
    }
}

- (void)notifyMoveOrResizeStarted
{
    if (_browser) {
        _browser->GetHost()->NotifyMoveOrResizeStarted();
    }
}

#pragma mark - Paint (OSR)

- (void)onPaintWithType:(CefRenderHandler::PaintElementType)type
             dirtyRects:(const CefRenderHandler::RectList&)dirtyRects
                 buffer:(const void*)buffer
                  width:(int)width
                 height:(int)height
{
    if (!_settings.offScreenRenderingEnabled || !_osrRenderer) return;
    _osrRenderer->onPaint(type, dirtyRects, buffer, width, height);
    _osrRenderer->scheduleRender();
}

- (void)onAcceleratedPaintWithType:(CefRenderHandler::PaintElementType)type
                        dirtyRects:(const CefRenderHandler::RectList&)dirtyRects
                              info:(const CefAcceleratedPaintInfo&)info
{
    if (!_settings.offScreenRenderingEnabled || !_osrRenderer) return;
    _osrRenderer->onAcceleratedPaint(type, dirtyRects, info);
    _osrRenderer->scheduleRender();
}

#pragma mark - Drag and Drop

/// Reset all drag and drop state.
- (void)resetDragDrop {
    _currentDragOp = NSDragOperationNone;
    _currentAllowedOps = NSDragOperationNone;
    _currentDragData = nullptr;
    if (_fileUTI) {
        _fileUTI = nil;
    }
    if (_pasteboard) {
        _pasteboard = nil;
    }
}

/// Fill the drag pasteboard with data from _currentDragData.
- (void)fillPasteboard {
    DCHECK(!_pasteboard);
    _pasteboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];

    [_pasteboard declareTypes:@[kCEFDragDummyPboardType] owner:self];

    if (_currentDragData->IsLink()) {
        [_pasteboard addTypes:@[NSPasteboardTypeURL, kNSURLTitlePboardType]
                        owner:self];
    }

    size_t contentsSize = _currentDragData->GetFileContents(nullptr);

    if (contentsSize > 0) {
        std::string fileName = _currentDragData->GetFileName().ToString();
        size_t sep = fileName.find_last_of(".");
        CefString extension = fileName.substr(sep + 1);
        CefString mimeType = CefGetMimeType(extension);

        if (!mimeType.empty()) {
            CFStringRef mimeTypeCF = CFStringCreateWithCString(
                kCFAllocatorDefault, mimeType.ToString().c_str(), kCFStringEncodingUTF8);
            _fileUTI = (__bridge_transfer NSString*)UTTypeCreatePreferredIdentifierForTag(
                kUTTagClassMIMEType, mimeTypeCF, nullptr);
            CFRelease(mimeTypeCF);

            NSArray* fileUTIList = @[_fileUTI];
            NSString* fileURLPromiseType =
                (__bridge NSString*)kPasteboardTypeFileURLPromise;
            [_pasteboard addTypes:@[fileURLPromiseType] owner:self];
            [_pasteboard setPropertyList:fileUTIList forType:fileURLPromiseType];
            [_pasteboard addTypes:fileUTIList owner:self];
        }
    }

    if (!_currentDragData->GetFragmentText().empty()) {
        [_pasteboard addTypes:@[NSPasteboardTypeString] owner:self];
    }
}

/// Populate CefDragData from pasteboard for incoming drags.
- (void)populateDropData:(CefRefPtr<CefDragData>)data
          fromPasteboard:(NSPasteboard*)pboard {
    DCHECK(data);
    DCHECK(pboard);
    DCHECK(data && !data->IsReadOnly());
    NSArray* types = [pboard types];

    if ([types containsObject:NSPasteboardTypeString]) {
        data->SetFragmentText(
            [[pboard stringForType:NSPasteboardTypeString] UTF8String]);
    }

    if ([types containsObject:NSPasteboardTypeFileURL]) {
        NSArray* files = [pboard propertyListForType:NSPasteboardTypeFileURL];
        if ([files isKindOfClass:[NSArray class]] && [files count]) {
            for (NSUInteger i = 0; i < [files count]; ++i) {
                NSString* filename = [files objectAtIndex:i];
                BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:filename];
                if (exists) {
                    data->AddFile([filename UTF8String], CefString());
                }
            }
        }
    }
}

/// Provide data lazily for the given pasteboard type.
- (void)pasteboard:(NSPasteboard*)pboard provideDataForType:(NSString*)type {
    if (!_currentDragData)
        return;

    if ([type isEqualToString:NSPasteboardTypeURL]) {
        DCHECK(_currentDragData->IsLink());
        NSString* strUrl =
            [NSString stringWithUTF8String:_currentDragData->GetLinkURL().ToString().c_str()];
        NSURL* url = [NSURL URLWithString:strUrl];
        [url writeToPasteboard:pboard];
    } else if ([type isEqualToString:kNSURLTitlePboardType]) {
        NSString* strTitle =
            [NSString stringWithUTF8String:_currentDragData->GetLinkTitle().ToString().c_str()];
        [pboard setString:strTitle forType:kNSURLTitlePboardType];
    } else if ([type isEqualToString:_fileUTI]) {
        size_t size = _currentDragData->GetFileContents(nullptr);
        DCHECK_GT(size, 0U);
        CefRefPtr<BytesWriteHandler> handler = new BytesWriteHandler(size);
        CefRefPtr<CefStreamWriter> writer =
            CefStreamWriter::CreateForHandler(handler.get());
        _currentDragData->GetFileContents(writer);
        DCHECK_EQ(handler->GetDataSize(), static_cast<int64_t>(size));
        [pboard setData:[NSData dataWithBytes:handler->GetData()
                                       length:handler->GetDataSize()]
                forType:_fileUTI];
    } else if ([type isEqualToString:NSPasteboardTypeString]) {
        NSString* strText =
            [NSString stringWithUTF8String:_currentDragData->GetFragmentText().ToString().c_str()];
        [pboard setString:strText forType:NSPasteboardTypeString];
    } else if ([type isEqualToString:kCEFDragDummyPboardType]) {
        [pboard setData:[NSData data] forType:kCEFDragDummyPboardType];
    }
}

/// Handle file promise drops to a destination folder.
- (NSArray*)namesOfPromisedFilesDroppedAtDestination:(NSURL*)dropDest {
    if (![dropDest isFileURL])
        return nil;
    if (!_currentDragData)
        return nil;

    size_t expectedSize = _currentDragData->GetFileContents(nullptr);
    if (expectedSize == 0)
        return nil;

    std::string path = [[dropDest path] UTF8String];
    path.append("/");
    path.append(_currentDragData->GetFileName().ToString());

    CefRefPtr<CefStreamWriter> writer = CefStreamWriter::CreateForFile(path);
    if (!writer)
        return nil;

    if (_currentDragData->GetFileContents(writer) != expectedSize)
        return nil;

    return @[[NSString stringWithUTF8String:path.c_str()]];
}

/// Create NSImage from CefImage for drag visuals.
- (NSImage*)createNSImageFromCefImage:(CefRefPtr<CefImage>)cefImage
                          scaleFactor:(float)scaleFactor {
    if (!cefImage || cefImage->IsEmpty())
        return nil;

    int pixelWidth = 0;
    int pixelHeight = 0;
    CefRefPtr<CefBinaryValue> bitmapData = cefImage->GetAsBitmap(
        scaleFactor, CEF_COLOR_TYPE_RGBA_8888, CEF_ALPHA_TYPE_PREMULTIPLIED,
        pixelWidth, pixelHeight);

    if (!bitmapData || bitmapData->GetSize() == 0)
        return nil;

    size_t dataSize = bitmapData->GetSize();
    std::vector<unsigned char> buffer(dataSize);
    bitmapData->GetData(&buffer[0], dataSize, 0);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        &buffer[0], pixelWidth, pixelHeight, 8, pixelWidth * 4, colorSpace,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);

    if (!context) {
        CGColorSpaceRelease(colorSpace);
        return nil;
    }

    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);

    if (!cgImage)
        return nil;

    NSSize imageSize = NSMakeSize(pixelWidth / scaleFactor, pixelHeight / scaleFactor);
    NSImage* nsImage = [[NSImage alloc] initWithCGImage:cgImage size:imageSize];
    CGImageRelease(cgImage);

    return nsImage;
}

/// Create a placeholder drag image when no real image is available.
- (NSImage*)createPlaceholderDragImage:(NSSize)size {
    NSImage* image = [[NSImage alloc] initWithSize:size];
    [image lockFocus];
    [[NSColor colorWithCalibratedWhite:0.5 alpha:0.5] setFill];
    NSRectFill(NSMakeRect(0, 0, size.width, size.height));
    [[NSColor colorWithCalibratedWhite:0.3 alpha:0.8] setStroke];
    NSBezierPath* path =
        [NSBezierPath bezierPathWithRect:NSMakeRect(0.5, 0.5, size.width - 1, size.height - 1)];
    [path setLineWidth:1.0];
    [path stroke];
    [image unlockFocus];
    return image;
}

- (BOOL)startDraggingWithDragData:(CefRefPtr<CefDragData>)dragData
                       allowedOps:(CefRenderHandler::DragOperationsMask)allowedOps
                                x:(int)x
                                y:(int)y
{
    DCHECK(!_pasteboard);
    DCHECK(!_fileUTI);
    DCHECK(!_currentDragData.get());

    [self resetDragDrop];

    _currentAllowedOps = static_cast<NSDragOperation>(allowedOps);
    _currentDragData = dragData;

    [self fillPasteboard];

    NSEvent* currentEvent = [[NSApplication sharedApplication] currentEvent];
    NSWindow* window = [self window];
    NSPoint windowLocation = [window mouseLocationOutsideOfEventStream];
    NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];

    NSEvent* dragEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                                            location:windowLocation
                                       modifierFlags:NSEventMaskLeftMouseDragged
                                           timestamp:[currentEvent timestamp]
                                        windowNumber:[window windowNumber]
                                             context:nil
                                         eventNumber:0
                                          clickCount:1
                                            pressure:1.0];

    NSImage* dragImage = nil;
    CefPoint hotspot = {0, 0};

    if (dragData->HasImage()) {
        CefRefPtr<CefImage> cefImage = dragData->GetImage();
        if (cefImage && !cefImage->IsEmpty()) {
            dragImage = [self createNSImageFromCefImage:cefImage scaleFactor:_deviceScaleFactor];
            hotspot = dragData->GetImageHotspot();
        }
    }

    if (!dragImage) {
        NSSize placeholderSize = NSMakeSize(100, 30);
        dragImage = [self createPlaceholderDragImage:placeholderSize];
        hotspot.x = static_cast<int>(placeholderSize.width / 2);
        hotspot.y = static_cast<int>(placeholderSize.height / 2);
    }

    NSSize imageSize = [dragImage size];

    NSPasteboardItem* pasteboardItem = [[NSPasteboardItem alloc] init];
    for (NSString* type in [_pasteboard types]) {
        NSData* data = [_pasteboard dataForType:type];
        if (data) {
            [pasteboardItem setData:data forType:type];
        } else {
            NSString* str = [_pasteboard stringForType:type];
            if (str) {
                [pasteboardItem setString:str forType:type];
            } else {
                id plist = [_pasteboard propertyListForType:type];
                if (plist) {
                    [pasteboardItem setPropertyList:plist forType:type];
                }
            }
        }
    }

    NSDraggingItem* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboardItem];

    NSRect draggingFrame = NSMakeRect(
        viewLocation.x - hotspot.x,
        viewLocation.y - imageSize.height + hotspot.y,
        imageSize.width,
        imageSize.height);
    dragItem.draggingFrame = draggingFrame;

    __block NSImage* blockDragImage = dragImage;
    __block NSSize blockImageSize = imageSize;
    dragItem.imageComponentsProvider = ^NSArray<NSDraggingImageComponent*>* {
        NSDraggingImageComponent* component = [[NSDraggingImageComponent alloc]
            initWithKey:NSDraggingImageComponentIconKey];
        component.contents = blockDragImage;
        component.frame = NSMakeRect(0, 0, blockImageSize.width, blockImageSize.height);
        return @[component];
    };

    NSDraggingSession* session = [self beginDraggingSessionWithItems:@[dragItem]
                                                               event:dragEvent
                                                              source:self];
    session.animatesToStartingPositionsOnCancelOrFail = YES;
    session.draggingFormation = NSDraggingFormationNone;

    return YES;
}

- (void)updateDragCursor:(CefRenderHandler::DragOperation)operation {
    _currentDragOp = static_cast<NSDragOperation>(operation);
}

#pragma mark - NSDraggingSource

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
    switch (context) {
    case NSDraggingContextOutsideApplication:
        return _currentAllowedOps;
    case NSDraggingContextWithinApplication:
    default:
        return _currentAllowedOps;
    }
}

- (void)draggingSession:(NSDraggingSession*)session
           endedAtPoint:(NSPoint)screenPoint
              operation:(NSDragOperation)operation {
    if (!_browser) return;

    if (operation == (NSDragOperationMove | NSDragOperationCopy)) {
        operation &= ~NSDragOperationMove;
    }

    NSPoint windowPoint = [[self window] convertPointFromScreen:screenPoint];
    NSPoint viewPoint = [self convertPoint:windowPoint fromView:nil];
    NSRect viewFrame = [self frame];
    viewPoint.y = viewFrame.size.height - viewPoint.y;

    CefRenderHandler::DragOperation op =
        static_cast<CefRenderHandler::DragOperation>(operation);
    _browser->GetHost()->DragSourceEndedAt(viewPoint.x, viewPoint.y, op);
    _browser->GetHost()->DragSourceSystemDragEnded();
    [self resetDragDrop];
}

#pragma mark - NSDraggingDestination

/// Create CefMouseEvent from drag info with proper coordinate conversion.
- (CefMouseEvent)createCefMouseEventFromDragInfo:(id<NSDraggingInfo>)info {
    CefMouseEvent cefEvent;

    NSPoint windowPoint = [info draggingLocation];
    NSPoint viewPoint = [self convertPoint:windowPoint fromView:nil];
    NSRect viewFrame = [self frame];
    viewPoint.y = viewFrame.size.height - viewPoint.y;

    viewPoint = [self convertPointToBacking:viewPoint];

    cefEvent.x = ScreenUtil::DeviceToLogical(static_cast<int>(viewPoint.x), _deviceScaleFactor);
    cefEvent.y = ScreenUtil::DeviceToLogical(static_cast<int>(viewPoint.y), _deviceScaleFactor);
    cefEvent.modifiers = static_cast<uint32_t>([NSEvent modifierFlags]);

    return cefEvent;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
    if (!_browser) return NSDragOperationNone;

    CefRefPtr<CefDragData> dragData;
    if (!_currentDragData) {
        dragData = CefDragData::Create();
        [self populateDropData:dragData fromPasteboard:[info draggingPasteboard]];
    } else {
        dragData = _currentDragData->Clone();
        dragData->ResetFileContents();
    }

    CefMouseEvent event = [self createCefMouseEventFromDragInfo:info];

    NSDragOperation mask = [info draggingSourceOperationMask];
    CefBrowserHost::DragOperationsMask allowedOps =
        static_cast<CefBrowserHost::DragOperationsMask>(mask);

    _browser->GetHost()->DragTargetDragEnter(dragData, event, allowedOps);
    _browser->GetHost()->DragTargetDragOver(event, allowedOps);

    _currentDragOp = NSDragOperationCopy;
    return _currentDragOp;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)info {
    if (!_browser) return NSDragOperationNone;

    CefMouseEvent event = [self createCefMouseEventFromDragInfo:info];

    NSDragOperation mask = [info draggingSourceOperationMask];
    CefBrowserHost::DragOperationsMask allowedOps =
        static_cast<CefBrowserHost::DragOperationsMask>(mask);

    _browser->GetHost()->DragTargetDragOver(event, allowedOps);

    return _currentDragOp;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    if (_browser) {
        _browser->GetHost()->DragTargetDragLeave();
    }
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)info {
    if (!_browser) return NO;

    CefMouseEvent event = [self createCefMouseEventFromDragInfo:info];
    _browser->GetHost()->DragTargetDrop(event);

    return YES;
}

#pragma mark - IME

- (void)onImeCompositionRangeChangedWithRange:(const CefRange&)selectionRange
                              characterBounds:(const CefRenderHandler::RectList&)characterBounds
{
    if (_textInputClient) {
        [_textInputClient ChangeCompositionRange:selectionRange character_bounds:characterBounds];
    }
}

- (void)onFocusOnEditableFieldChanged:(BOOL)focusOnEditableField
{
    _focusOnEditableField = focusOnEditableField;
}

- (BOOL)isFocusOnEditableField
{
    return _focusOnEditableField;
}

#pragma mark - Mouse Events

/// Send mouse click event to CEF.
- (void)sendMouseClick:(NSEvent*)event
                button:(CefBrowserHost::MouseButtonType)type
                  isUp:(bool)isUp {
    if (!_browser) return;

    CefMouseEvent mouseEvent = [self createCefMouseEventFromNSEvent:event];
    _browser->GetHost()->SendMouseClickEvent(
        mouseEvent, type, isUp, static_cast<int>([event clickCount]));
}

- (void)mouseDown:(NSEvent*)event {
    [[self window] makeFirstResponder:self];

    if (!CGRectEqualToRect(_nonDragArea, CGRectZero)) {
        NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
        NSRect viewFrame = [self frame];
        CGPoint point = CGPointMake(location.x, viewFrame.size.height - location.y);
        if (!CGRectContainsPoint(_nonDragArea, point)) {
            _movingWindow = true;
            [[self window] performWindowDragWithEvent:event];
            return;
        }
    }

    [self sendMouseClick:event button:MBT_LEFT isUp:false];
}

- (void)mouseUp:(NSEvent*)event {
    if (_movingWindow) {
        _movingWindow = false;
        return;
    }
    [self sendMouseClick:event button:MBT_LEFT isUp:true];
}

- (void)rightMouseDown:(NSEvent*)event {
    [self sendMouseClick:event button:MBT_RIGHT isUp:false];
}

- (void)rightMouseUp:(NSEvent*)event {
    [self sendMouseClick:event button:MBT_RIGHT isUp:true];
}

- (void)otherMouseDown:(NSEvent*)event {
    [self sendMouseClick:event button:MBT_MIDDLE isUp:false];
}

- (void)otherMouseUp:(NSEvent*)event {
    [self sendMouseClick:event button:MBT_MIDDLE isUp:true];
}

- (void)mouseMoved:(NSEvent*)event {
    if (!_browser) return;

    CefMouseEvent cefEvent = [self createCefMouseEventFromNSEvent:event];
    _browser->GetHost()->SendMouseMoveEvent(cefEvent, false);
}

- (void)mouseDragged:(NSEvent*)event {
    if (_movingWindow) return;
    [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)mouseEntered:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)mouseExited:(NSEvent*)event {
    if (!_browser) return;

    CefMouseEvent cefEvent = [self createCefMouseEventFromNSEvent:event];
    _browser->GetHost()->SendMouseMoveEvent(cefEvent, true);
}

/// Handle scroll wheel phase ended/cancelled from NSEvent monitor.
- (void)shortCircuitScrollWheelEvent:(NSEvent*)event
{
    if ([event phase] != NSEventPhaseEnded &&
        [event phase] != NSEventPhaseCancelled)
        return;

    [self sendScrollWheelEvent:event];

    if (_endWheelMonitor) {
        [NSEvent removeMonitor:_endWheelMonitor];
        _endWheelMonitor = nil;
    }
}

/// Send scroll wheel event to CEF using raw CGEvent deltas.
- (void)sendScrollWheelEvent:(NSEvent*)event
{
    if (!_browser) return;

    CGEventRef cgEvent = [event CGEvent];
    DCHECK(cgEvent);

    int deltaX = static_cast<int>(
        CGEventGetIntegerValueField(cgEvent, kCGScrollWheelEventPointDeltaAxis2));
    int deltaY = static_cast<int>(
        CGEventGetIntegerValueField(cgEvent, kCGScrollWheelEventPointDeltaAxis1));

    CefMouseEvent mouseEvent = [self createCefMouseEventFromNSEvent:event];
    _browser->GetHost()->SendMouseWheelEvent(mouseEvent, deltaX, deltaY);
}

- (void)scrollWheel:(NSEvent*)event
{
    if ([event phase] == NSEventPhaseBegan && !_endWheelMonitor) {
        _endWheelMonitor = [NSEvent
            addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
                                        handler:^(NSEvent* blockEvent) {
                                            [self shortCircuitScrollWheelEvent:blockEvent];
                                            return blockEvent;
                                        }];
    }
    [self sendScrollWheelEvent:event];
}

#pragma mark - Edit Menu Commands

- (void)cut:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->Cut();
}

- (void)copy:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->Copy();
}

- (void)paste:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->Paste();
}

- (void)selectAll:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->SelectAll();
}

- (void)undo:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->Undo();
}

- (void)redo:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->Redo();
}

- (void)delete:(id)sender
{
    if (!_browser) return;
    _browser->GetFocusedFrame()->Delete();
}

#pragma mark - Keyboard Events

- (void)keyDown:(NSEvent*)event
{
    if (!_browser || !_textInputContextOsrMac) return;

    if ([event type] != NSEventTypeFlagsChanged) {
        if (_textInputClient) {
            [_textInputClient HandleKeyEventBeforeTextInputClient:event];
            [_textInputContextOsrMac handleEvent:event];
            CefKeyEvent keyEvent = [self createCefKeyEventFromNSEvent:event];
            [_textInputClient HandleKeyEventAfterTextInputClient:keyEvent];
        }
    }
}

- (void)keyUp:(NSEvent*)event
{
    if (!_browser) return;

    CefKeyEvent cefEvent = [self createCefKeyEventFromNSEvent:event];
    cefEvent.type = KEYEVENT_KEYUP;
    _browser->GetHost()->SendKeyEvent(cefEvent);
}

- (void)flagsChanged:(NSEvent*)event
{
    if ([self isKeyUpEvent:event]) {
        [self keyUp:event];
    } else {
        [self keyDown:event];
    }
}

/// Determine if a modifier key event represents key-up.
- (BOOL)isKeyUpEvent:(NSEvent*)event
{
    if ([event type] != NSEventTypeFlagsChanged)
        return [event type] == NSEventTypeKeyUp;

    switch ([event keyCode]) {
    case 54: // Right Command
    case 55: // Left Command
        return ([event modifierFlags] & NSEventModifierFlagCommand) == 0;
    case 57: // Capslock
        return ([event modifierFlags] & NSEventModifierFlagCapsLock) == 0;
    case 56: // Left Shift
    case 60: // Right Shift
        return ([event modifierFlags] & NSEventModifierFlagShift) == 0;
    case 58: // Left Alt
    case 61: // Right Alt
        return ([event modifierFlags] & NSEventModifierFlagOption) == 0;
    case 59: // Left Ctrl
    case 62: // Right Ctrl
        return ([event modifierFlags] & NSEventModifierFlagControl) == 0;
    case 63: // Function
        return ([event modifierFlags] & NSEventModifierFlagFunction) == 0;
    }
    return NO;
}

/// Determine if an event is from the numeric keypad.
- (BOOL)isKeyPadEvent:(NSEvent*)event
{
    if ([event modifierFlags] & NSEventModifierFlagNumericPad)
        return YES;

    switch ([event keyCode]) {
    case 71: // Clear
    case 81: // =
    case 75: // /
    case 67: // *
    case 78: // -
    case 69: // +
    case 76: // Enter
    case 65: // .
    case 82: // 0
    case 83: // 1
    case 84: // 2
    case 85: // 3
    case 86: // 4
    case 87: // 5
    case 88: // 6
    case 89: // 7
    case 91: // 8
    case 92: // 9
        return YES;
    }
    return NO;
}

#pragma mark - Event Helpers

- (CefMouseEvent)createCefMouseEventFromNSEvent:(NSEvent*)event
{
    CefMouseEvent cefEvent;

    // |location| is in OS X view coordinates.
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    NSRect bounds = [self bounds];
    location.y = bounds.size.height - location.y;

    // Convert to device coordinates.
    location = [self convertPointToBacking:location];

    // Convert to DIP coordinates.
    cefEvent.x = ScreenUtil::DeviceToLogical(static_cast<int>(location.x), _deviceScaleFactor);
    cefEvent.y = ScreenUtil::DeviceToLogical(static_cast<int>(location.y), _deviceScaleFactor);
    cefEvent.modifiers = [self getCefModifiersFromNSEvent:event];

    return cefEvent;
}

- (CefKeyEvent)createCefKeyEventFromNSEvent:(NSEvent*)event
{
    CefKeyEvent cefEvent;

    if ([event type] == NSEventTypeKeyDown || [event type] == NSEventTypeKeyUp) {
        NSString* s = [event characters];
        if ([s length] > 0) {
            cefEvent.character = [s characterAtIndex:0];
        }

        s = [event charactersIgnoringModifiers];
        if ([s length] > 0) {
            cefEvent.unmodified_character = [s characterAtIndex:0];
        }
    }

    if ([event type] == NSEventTypeFlagsChanged) {
        cefEvent.character = 0;
        cefEvent.unmodified_character = 0;
    }

    cefEvent.native_key_code = [event keyCode];
    cefEvent.modifiers = [self getCefModifiersFromNSEvent:event];

    return cefEvent;
}

- (int)getCefModifiersFromNSEvent:(NSEvent*)event
{
    int modifiers = 0;

    if ([event modifierFlags] & NSEventModifierFlagControl)
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if ([event modifierFlags] & NSEventModifierFlagShift)
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if ([event modifierFlags] & NSEventModifierFlagOption)
        modifiers |= EVENTFLAG_ALT_DOWN;
    if ([event modifierFlags] & NSEventModifierFlagCommand)
        modifiers |= EVENTFLAG_COMMAND_DOWN;
    if ([event modifierFlags] & NSEventModifierFlagCapsLock)
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;

    if ([event type] == NSEventTypeKeyUp || [event type] == NSEventTypeKeyDown ||
        [event type] == NSEventTypeFlagsChanged) {
        if ([self isKeyPadEvent:event])
            modifiers |= EVENTFLAG_IS_KEY_PAD;
    }

    switch ([event type]) {
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeLeftMouseDown:
    case NSEventTypeLeftMouseUp:
        modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
        break;
    case NSEventTypeRightMouseDragged:
    case NSEventTypeRightMouseDown:
    case NSEventTypeRightMouseUp:
        modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
        break;
    case NSEventTypeOtherMouseDragged:
    case NSEventTypeOtherMouseDown:
    case NSEventTypeOtherMouseUp:
        modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
        break;
    default:
        break;
    }

    return modifiers;
}

#pragma mark - Event Handlers

- (BOOL)onCursorChangeWithBrowser:(CefRefPtr<CefBrowser>)browser
                           cursor:(CefCursorHandle)cursor
                             type:(cef_cursor_type_t)type
                 customCursorInfo:(const CefCursorInfo&)customCursorInfo
{
    if (cursor) {
        [(__bridge NSCursor*)cursor set];
    }
    return YES;
}

- (void)onTitleChangeWithBrowserId:(int)browserId title:(const std::string&)title
{
    // Can be overridden by subclass
}

- (void)onUrlChangeWithBrowserId:(int)browserId
                          oldUrl:(const std::string&)oldUrl
                             url:(const std::string&)url
{
    _settings.url = url;
}

- (void)onLoadingStateChangeWithBrowserId:(int)browserId
                                isLoading:(BOOL)isLoading
                                canGoBack:(BOOL)canGoBack
                             canGoForward:(BOOL)canGoForward
{
    // Can be overridden by subclass
}

- (void)onLoadStartWithUrl:(const std::string&)url
{
    _isLoading = true;
}

- (void)onLoadEndWithUrl:(const std::string&)url
{
    _isLoading = false;

    // Execute cached JS codes
    if (!_cachedJsCodes.empty() && _browser) {
        CefRefPtr<CefFrame> frame = _browser->GetMainFrame();
        if (frame) {
            for (const auto& script : _cachedJsCodes) {
                frame->ExecuteJavaScript(CefString(script), frame->GetURL(), 0);
            }
        }
        _cachedJsCodes.clear();
    }
}

- (void)onLoadErrorWithBrowserId:(int)browserId
                       errorText:(const std::string&)errorText
                       failedUrl:(const std::string&)failedUrl
{
    _isLoading = false;
}

- (void)onAfterCreatedWithBrowserId:(int)browserId
{
    if (_client) {
        _browser = _client->GetBrowser();
    }

    // Initialize IME client
    if (_browser && _settings.offScreenRenderingEnabled) {
        _textInputClient = [[OsrCefTextInputClient alloc] initWithBrowser:_browser];
        _textInputContextOsrMac = [[NSTextInputContext alloc] initWithClient:_textInputClient];
    }

    // Setup key event monitor
    [self setupKeyEventMonitor];

    // Execute pending tasks
    for (auto& task : _taskListAfterCreated) {
        if (task) {
            task();
        }
    }
    _taskListAfterCreated.clear();
}

- (void)onBeforeCloseWithBrowserId:(int)browserId
{
    // Cleanup IME
    if (_textInputClient) {
        [_textInputClient detach];
        _textInputClient = nil;
    }
    _textInputContextOsrMac = nil;

    _browser = nullptr;

    // Notify the owner that the browser has been destroyed.
    // Following cefclient OSR pattern: browser destroys first, then native window closes.
    if (self.browserCloseCallback) {
        self.browserCloseCallback();
    }
}

- (void)onProcessMessageReceivedWithBrowserId:(int)browserId
                                  messageName:(const std::string&)messageName
                                     jsonArgs:(const std::string&)jsonArgs
{
    // Can be overridden by subclass
}

#pragma mark - Keyboard Shortcuts

- (BOOL)handleShortcutKeyWithKeyCode:(int)keyCode modifiers:(uint32_t)modifiers
{
    bool cmdPressed = (modifiers & EVENTFLAG_COMMAND_DOWN) != 0;
    bool shiftPressed = (modifiers & EVENTFLAG_SHIFT_DOWN) != 0;
    bool optPressed = (modifiers & EVENTFLAG_ALT_DOWN) != 0;

    // Cmd+R: Reload
    if (cmdPressed && keyCode == 15) {  // 15 = 'R' on macOS
        [self refresh];
        return YES;
    }

    // F12 (keyCode 111) or Cmd+Opt+I: Open DevTools
    if (keyCode == 111 || (cmdPressed && optPressed && keyCode == 34)) {  // 34 = 'I' on macOS
        [self openDevTools];
        return YES;
    }

    return NO;
}

#pragma mark - NSView Overrides

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    if (_browser) {
        _browser->GetHost()->SetFocus(true);
    }
    return YES;
}

- (BOOL)resignFirstResponder
{
    if (_browser) {
        _browser->GetHost()->SetFocus(false);
    }
    return YES;
}

- (void)cut:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->Cut();
    }
}

- (void)copy:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->Copy();
    }
}

- (void)paste:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->Paste();
    }
}

- (void)selectAll:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->SelectAll();
    }
}

- (void)undo:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->Undo();
    }
}

- (void)redo:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->Redo();
    }
}

- (void)delete:(id)sender {
    if (!_settings.offScreenRenderingEnabled) return;
    if (_browser) {
        _browser->GetFocusedFrame()->Delete();
    }
}

- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];

    if (_osrRenderer) {
        _osrRenderer->setBounds(0, 0, static_cast<int>(newSize.width), static_cast<int>(newSize.height));
    }

    if (_browser) {
        _browser->GetHost()->WasResized();
    }
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];

    NSWindow* window = [self window];
    if (window) {
        _deviceScaleFactor = [window backingScaleFactor];

        // Observe backing scale factor changes
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidChangeBackingProperties:)
                                                     name:NSWindowDidChangeBackingPropertiesNotification
                                                   object:window];
    }
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
    NSWindow* window = [notification object];
    if (window) {
        float newScaleFactor = [window backingScaleFactor];
        if (newScaleFactor != _deviceScaleFactor) {
            [self setDeviceScaleFactor:newScaleFactor];
        }
    }
}

- (void)viewWillMoveToWindow:(NSWindow*)newWindow
{
    NSWindow* oldWindow = [self window];
    if (oldWindow) {
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:NSWindowDidChangeBackingPropertiesNotification
                                                      object:oldWindow];
    }
}

- (void)drawRect:(NSRect)dirtyRect
{
    if (_osrRenderer) {
        _osrRenderer->render();
    }
}

@end
