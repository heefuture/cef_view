#ifndef CEFVIEWCLIENTDELEGATE_H
#define CEFVIEWCLIENTDELEGATE_H
#pragma once

#import <Cocoa/Cocoa.h>
#include "client/CefViewClient.h"
#include "client/CefViewClientDelegateInterface.h"
#include "include/cef_base.h"
#include <memory>

namespace cefview {
class CefJsBridgeBrowser;
}

/// Observer protocol for CEF browser callbacks.
/// @required methods are high-frequency rendering/interaction callbacks that must be implemented.
/// @optional methods are page lifecycle events that observers can selectively handle.
@protocol CefWebViewObserver <NSObject>

@required

/// Handle CEF OnPaint callback for software rendering
- (void)onPaintWithType:(CefRenderHandler::PaintElementType)type
             dirtyRects:(const CefRenderHandler::RectList&)dirtyRects
                 buffer:(const void*)buffer
                  width:(int)width
                 height:(int)height;

/// Handle CEF OnAcceleratedPaint callback for GPU rendering
- (void)onAcceleratedPaintWithType:(CefRenderHandler::PaintElementType)type
                        dirtyRects:(const CefRenderHandler::RectList&)dirtyRects
                              info:(const CefAcceleratedPaintInfo&)info;

/// Get the root screen rectangle
- (BOOL)getRootScreenRect:(CefRect&)rect;

/// Get the view rectangle
- (BOOL)getViewRect:(CefRect&)rect;

/// Convert view coordinates to screen coordinates
- (BOOL)getScreenPointWithViewX:(int)viewX
                          viewY:(int)viewY
                        screenX:(int*)screenX
                        screenY:(int*)screenY;

/// Get screen information
- (BOOL)getScreenInfo:(CefScreenInfo&)screenInfo;

/// Handle cursor change event
- (BOOL)onCursorChangeWithBrowser:(CefRefPtr<CefBrowser>)browser
                           cursor:(CefCursorHandle)cursor
                             type:(cef_cursor_type_t)type
                 customCursorInfo:(const CefCursorInfo&)customCursorInfo;

/// Start a drag operation
- (BOOL)startDraggingWithDragData:(CefRefPtr<CefDragData>)dragData
                       allowedOps:(CefRenderHandler::DragOperationsMask)allowedOps
                                x:(int)x
                                y:(int)y;

/// Update drag cursor
- (void)updateDragCursor:(CefRenderHandler::DragOperation)operation;

/// Handle IME composition range change
- (void)onImeCompositionRangeChangedWithRange:(const CefRange&)selectionRange
                              characterBounds:(const CefRenderHandler::RectList&)characterBounds;

/// Called when the editable state of the focused DOM node changes.
- (void)onEditableFocusChanged:(CefRefPtr<CefProcessMessage>)message;

/// Handle shortcut key events (Cmd+R, F12, etc.)
- (BOOL)handleShortcutKeyWithKeyCode:(int)keyCode modifiers:(uint32_t)modifiers;

@optional

/// Browser creation complete event
- (void)onAfterCreatedWithBrowserId:(int)browserId;

/// Browser creation complete event with initial URL
- (void)onAfterCreatedWithBrowserId:(int)browserId url:(const std::string&)url;

/// Handle custom process messages from renderer
- (std::string)onProcessMessageReceived:(const std::string&)messageName
                                  param:(const std::string&)param;

/// Browser close event
- (void)onBeforeCloseWithBrowserId:(int)browserId;

/// Title change event
- (void)onTitleChangeWithBrowserId:(int)browserId title:(const std::string&)title;

/// URL change event
- (void)onUrlChangeWithBrowserId:(int)browserId
                          oldUrl:(const std::string&)oldUrl
                             url:(const std::string&)url;

/// Loading state change event
- (void)onLoadingStateChangeWithBrowserId:(int)browserId
                                isLoading:(BOOL)isLoading
                                canGoBack:(BOOL)canGoBack
                             canGoForward:(BOOL)canGoForward;

/// Load start event
- (void)onLoadStartWithUrl:(const std::string&)url;

/// Load end event
- (void)onLoadEndWithUrl:(const std::string&)url;

/// Load error event
- (void)onLoadErrorWithBrowserId:(int)browserId
                       errorText:(const std::string&)errorText
                       failedUrl:(const std::string&)failedUrl;

/// Open developer tools
- (BOOL)openDevTools;

@end

namespace cefview {

// C++ delegate class that bridges CEF callbacks to Objective-C CefWebView
class CefViewClientDelegate : public CefViewClientDelegateInterface {
public:
    explicit CefViewClientDelegate(id<CefWebViewObserver> observer);
    ~CefViewClientDelegate();

protected:
#pragma region CefClient
    virtual bool onProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId sourceProcess,
                                          CefRefPtr<CefProcessMessage> message) override;

    virtual void onEditableFocusChanged(CefRefPtr<CefProcessMessage> message) override;
#pragma endregion // CefClient

#pragma region CefContextMenuHandler
    virtual void onBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) override;

    virtual bool onContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefContextMenuParams> params,
                                      int commandId,
                                      CefContextMenuHandler::EventFlags eventFlags) override;
#pragma endregion // CefContextMenuHandler

#pragma region CefDisplayHandler
    virtual void onAddressChange(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 const CefString& url) override;

    virtual void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;

    virtual bool onCursorChange(CefRefPtr<CefBrowser> browser,
                                CefCursorHandle cursor,
                                cef_cursor_type_t type,
                                const CefCursorInfo& customCursorInfo) override;

    virtual bool onConsoleMessage(CefRefPtr<CefBrowser> browser,
                                  cef_log_severity_t level,
                                  const CefString& message,
                                  const CefString& source,
                                  int line) override;
#pragma endregion // CefDisplayHandler

#pragma region CefDownloadHandler
    virtual bool onBeforeDownload(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefDownloadItem> downloadItem,
                                  const CefString& suggestedName,
                                  CefRefPtr<CefBeforeDownloadCallback> callback) override;

    virtual void onDownloadUpdated(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefDownloadItem> downloadItem,
                                   CefRefPtr<CefDownloadItemCallback> callback) override;
#pragma endregion // CefDownloadHandler

#pragma region CefDragHandler
    virtual bool onDragEnter(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefDragData> dragData,
                             CefRenderHandler::DragOperationsMask mask) override;
#pragma endregion // CefDragHandler

#pragma region CefKeyboardHandler
    virtual bool onPreKeyEvent(CefRefPtr<CefBrowser> browser,
                               const CefKeyEvent& event,
                               CefEventHandle osEvent,
                               bool* isKeyboardShortcut) override;

    virtual bool onKeyEvent(CefRefPtr<CefBrowser> browser,
                           const CefKeyEvent& event,
                           CefEventHandle osEvent) override;
#pragma endregion // CefKeyboardHandler

#pragma region CefLifeSpanHandler
    virtual bool onBeforePopup(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               int popupId,
                               const CefString& targetUrl,
                               const CefString& targetFrameName,
                               CefLifeSpanHandler::WindowOpenDisposition targetDisposition,
                               bool userGesture,
                               const CefPopupFeatures& popupFeatures,
                               CefWindowInfo& windowInfo,
                               CefRefPtr<CefClient>& client,
                               CefBrowserSettings& settings,
                               CefRefPtr<CefDictionaryValue>& extraInfo,
                               bool* noJavascriptAccess) override;

    virtual void onAfterCreated(CefRefPtr<CefBrowser> browser) override;

    virtual void onBeforeClose(CefRefPtr<CefBrowser> browser) override;
#pragma endregion // CefLifeSpanHandler

#pragma region CefLoadHandler
    virtual void onLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                      bool isLoading,
                                      bool canGoBack,
                                      bool canGoForward) override;

    virtual void onLoadStart(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame,
                            CefLoadHandler::TransitionType transitionType) override;

    virtual void onLoadEnd(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          int httpStatusCode) override;

    virtual void onLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefLoadHandler::ErrorCode errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl) override;
#pragma endregion // CefLoadHandler

#pragma region CefRenderHandler
    virtual void onPaint(CefRefPtr<CefBrowser> browser,
                        CefRenderHandler::PaintElementType type,
                        const CefRenderHandler::RectList& dirtyRects,
                        const void* buffer,
                        int width,
                        int height) override;

    virtual void onAcceleratedPaint(CefRefPtr<CefBrowser> browser,
                                   CefRenderHandler::PaintElementType type,
                                   const CefRenderHandler::RectList& dirtyRects,
                                   const CefAcceleratedPaintInfo& info) override;

    virtual bool getRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

    virtual void getViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

    virtual bool getScreenPoint(CefRefPtr<CefBrowser> browser,
                               int viewX, int viewY,
                               int& screenX, int& screenY) override;

    virtual bool getScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screenInfo) override;

    virtual void onPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;

    virtual void onPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override;

    virtual bool startDragging(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefDragData> dragData,
                              CefRenderHandler::DragOperationsMask allowedOps,
                              int x, int y) override;

    virtual void updateDragCursor(CefRefPtr<CefBrowser> browser,
                                  CefRenderHandler::DragOperation operation) override;

    virtual void onImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
                                             const CefRange& selectionRange,
                                             const CefRenderHandler::RectList& characterBounds) override;
#pragma endregion // CefRenderHandler

#pragma region CefPermissionHandler
    virtual bool onShowPermissionPrompt(CefRefPtr<CefBrowser> browser,
                                       uint64_t promptId,
                                       const CefString& requestingOrigin,
                                       uint32_t requestedPermissions,
                                       CefRefPtr<CefPermissionPromptCallback> callback) override;
#pragma endregion // CefPermissionHandler

#pragma region CefRequestHandler
    virtual bool onBeforeBrowse(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               CefRefPtr<CefRequest> request,
                               bool userGesture,
                               bool isRedirect) override;

    virtual void onRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                          CefRequestHandler::TerminationStatus status,
                                          int errorCode,
                                          const CefString& errorString) override;
#pragma endregion // CefRequestHandler

protected:
    __weak id<CefWebViewObserver> _observer;
    CefString _url;
    bool _isDevtoolsOpened = false;
    std::shared_ptr<CefJsBridgeBrowser> _jsBridgeBrowser;
};

}  // namespace cefview

#endif  // CEFVIEWCLIENTDELEGATE_H
