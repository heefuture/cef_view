/**
 * @file        HelperProcess.mm
 * @brief       Entry point for macOS CEF helper sub-processes.
 *              Each helper app bundle (GPU, Renderer, Plugin, Alerts) uses this
 *              as its main function. The CEF framework is loaded dynamically via
 *              CefScopedLibraryLoader instead of linking directly.
 * @version     1.0
 * @date        2026.02.10
 * @copyright   Copyright (C) 2026 Tencent. All rights reserved.
 */

#include <string>

#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_library_loader.h"

#include <client/CefViewApp.h>
#include <client/CefViewAppDelegateRenderer.h>
#include <global/CefConfig.h>

namespace cefview {

static std::string GetProcessType(int argc, char* argv[]) {
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);
    if (!command_line->HasSwitch("type")) {
        return "browser";
    }
    return command_line->GetSwitchValue("type").ToString();
}

}  // namespace cefview

int main(int argc, char* argv[]) {
    // Load the CEF framework library at runtime.
    // This is required for macOS helper processes due to sandbox constraints.
    CefScopedLibraryLoader library_loader;
    if (!library_loader.LoadInHelper()) {
        return 1;
    }

    CefMainArgs main_args(argc, argv);
    cefview::CefConfig config;
    std::string process_type = cefview::GetProcessType(argc, argv);

    // Create CefViewApp with appropriate delegate based on process type.
    CefRefPtr<cefview::CefViewApp> app;
    if (process_type == "renderer") {
        auto renderer_delegate = std::make_shared<cefview::CefViewAppDelegateRenderer>();
        app = new cefview::CefViewApp(config, renderer_delegate);
    } else {
        app = new cefview::CefViewApp(config);
    }

    return CefExecuteProcess(main_args, app.get(), nullptr);
}
