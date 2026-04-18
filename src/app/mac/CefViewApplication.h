/**
 * @file        CefViewApplication.h
 * @brief       NSApplication subclass that conforms to CefAppProtocol
 * @version     1.0
 * @author      heefuture
 * @date        2026.04.18
 * @copyright
 */
#ifndef CEFVIEWAPPLICATION_H
#define CEFVIEWAPPLICATION_H
#pragma once

#import <Cocoa/Cocoa.h>

#include "include/cef_application_mac.h"

/**
 * Custom NSApplication required by CEF on macOS.
 *
 * CEF needs the host NSApplication to implement CefAppProtocol so it can
 * track whether the app is currently inside -sendEvent:. This is required
 * for correct nested event-loop handling (drag-and-drop, IME, popup menus)
 * and is asserted by debug builds of the CEF framework.
 */
@interface CefViewApplication : NSApplication <CefAppProtocol>

@end

#endif  // CEFVIEWAPPLICATION_H
