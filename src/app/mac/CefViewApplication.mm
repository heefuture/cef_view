/**
 * @file        CefViewApplication.mm
 * @brief       NSApplication subclass that conforms to CefAppProtocol
 * @version     1.0
 * @author      heefuture
 * @date        2026.04.18
 * @copyright
 */
#import "CefViewApplication.h"

#include "global/CefContext.h"

@implementation CefViewApplication {
    BOOL _handlingSendEvent;
}

- (BOOL)isHandlingSendEvent {
    return _handlingSendEvent;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
    _handlingSendEvent = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event {
    // Tell CEF we are inside -sendEvent: so nested message-loop work behaves
    // correctly. The scoper restores the previous value on destruction, which
    // is important because sendEvent: can be re-entered.
    CefScopedSendingEvent sendingEventScoper;
    [super sendEvent:event];
}

- (void)terminate:(id)sender {
    // The default -[NSApplication terminate:] calls exit() and bypasses the
    // CEF shutdown sequence, which can leave browsers un-closed and triggers
    // CHECK failures during process teardown. Quit the CEF message loop
    // instead and let main() drive the orderly shutdown.
    cefview::CefContext::instance().quitMessageLoop();
}

@end
