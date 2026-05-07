/**
 * @file        HelperProcess.mm
 * @brief       Entry point for macOS CEF helper sub-processes.
 *              Each helper app bundle (GPU, Renderer, Plugin, Alerts) uses this
 *              as its main function. The CEF framework is loaded dynamically via
 *              CefScopedLibraryLoader instead of linking directly.
 * @version     1.0
 * @date        2026.02.10
 * @copyright
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
    CefRefPtr<CefCommandLine> commandLine = CefCommandLine::CreateCommandLine();
    commandLine->InitFromArgv(argc, argv);
    if (!commandLine->HasSwitch("type")) {
        return "browser";
    }
    return commandLine->GetSwitchValue("type").ToString();
}

}  // namespace cefview

int main(int argc, char* argv[]) {
    // Load the CEF framework library at runtime.
    // This is required for macOS helper processes due to sandbox constraints.
    CefScopedLibraryLoader libraryLoader;
    if (!libraryLoader.LoadInHelper()) {
        return 1;
    }

    CefMainArgs mainArgs(argc, argv);
    cefview::CefConfig config;
    std::string processType = cefview::GetProcessType(argc, argv);

    // Create CefViewApp with appropriate delegate based on process type.
    // The delegate must outlive CefExecuteProcess because CefViewApp stores
    // it as a std::weak_ptr; a block-scoped shared_ptr would expire
    // immediately and OnWebKitInitialized would skip delegate callbacks,
    // leaving the `cefViewApp` JS bridge unregistered.
    std::shared_ptr<cefview::CefViewAppDelegateInterface> delegate;
    if (processType == "renderer") {
        delegate = std::make_shared<cefview::CefViewAppDelegateRenderer>();
    }
    CefRefPtr<cefview::CefViewApp> app = delegate
        ? new cefview::CefViewApp(config, delegate)
        : new cefview::CefViewApp(config);

    return CefExecuteProcess(mainArgs, app.get(), nullptr);
}
