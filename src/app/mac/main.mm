/**
 * @file        main.mm
 * @brief       macOS application entry point
 * @version     1.0
 * @author      heefuture
 * @date        2026.02.09
 * @copyright
 */
#import <Cocoa/Cocoa.h>

#include <memory>
#include <string>

#include "include/wrapper/cef_library_loader.h"

#include "global/CefConfig.h"
#include "global/CefContext.h"
#include "client/CefViewAppDelegateInterface.h"

#import "AppDelegate.h"
#import "CefViewApplication.h"

using namespace cefview;

int main(int argc, char* argv[]) {
    // CEF library loader must outlive @autoreleasepool to ensure the CEF
    // framework stays loaded until after CefShutdown completes.
    CefScopedLibraryLoader library_loader;
    if (!library_loader.LoadInMain()) {
        return 1;
    }

    @autoreleasepool {
        // Install the CefAppProtocol-conforming NSApplication subclass before
        // any other NSApp access. The first +sharedApplication call decides
        // the concrete NSApp class, so this must run first.
        [CefViewApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        // The browser process only needs a browser-side delegate; the
        // renderer-side delegate is registered inside the Helper bundle
        // entry point (see src/sub_process/HelperProcess.mm), because
        // macOS spawns renderer sub-processes as separate helper apps.
        std::shared_ptr<CefViewAppDelegateInterface> browserDelegate = nullptr;
        std::shared_ptr<CefViewAppDelegateInterface> rendererDelegate = nullptr;

        // Configure CEF
        CefConfig cefConfig;
        cefConfig.backgroundColor = 0x00000000;
        cefConfig.multiThreadedMessageLoop = false;  // macOS does not support multi-threaded message loop

        // Follow system language settings
        NSString* preferredLanguage = [[NSLocale preferredLanguages] firstObject];
        if (preferredLanguage) {
            std::string langStr = [preferredLanguage UTF8String];
            cefConfig.locale = langStr;
            cefConfig.acceptLanguageList = langStr;
            if (langStr.find("en") != 0) {
                cefConfig.acceptLanguageList += ",en-US;q=0.9,en;q=0.8";
            }
        }

        // Initialize CEF
        auto& context = CefContext::instance();
        int initResult = context.initialize(argc, argv, cefConfig, browserDelegate, rendererDelegate);
        if (initResult >= 0) {
            // Sub-process completed
            return initResult;
        }

        // Create app delegate and launch
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];

        // Activate the application
        [NSApp activateIgnoringOtherApps:YES];

        // CefRunMessageLoop integrates with NSRunLoop, replacing [NSApp run]
        context.runMessageLoop();

        // Explicitly shut down CEF on the main thread after the message loop exits.
        context.shutdown();
    }
    return 0;
}
