#include "CefContext.h"
#include <client/CefViewApp.h>
#include <client/CefViewAppDelegateInterface.h>
#include <utils/PathUtil.h>

#include "include/cef_app.h"

#include <string>

// Disable separate sub-process executable (browser.exe).
// When defined, the main process handles all CEF functionality.
#define SUB_PROCESS_DISABLED

namespace cefview {

static CefSettings getCefSettings(const CefConfig& config) {
    CefSettings settings;
    settings.multi_threaded_message_loop = config.multiThreadedMessageLoop;
    CefString(&settings.cookieable_schemes_list) = config.cookieableSchemesList;

    // Store cache data on disk.
    if (config.cachePath.empty()) {
        std::string cachePath = PathUtil::GetAppWorkingDirectory() + "/cache";
        CefString(&settings.cache_path) = CefString(cachePath);
    }
    else {
        CefString(&settings.cache_path) = CefString(config.cachePath);
    }

    // Set debug log file path.
    if (config.logFilePath.empty()) {
        std::string logPath = PathUtil::GetAppWorkingDirectory() + "/cef.log";
        CefString(&settings.log_file) = CefString(logPath);
    }
    else {
        CefString(&settings.log_file) = CefString(config.logFilePath);
    }

    // The resources(cef.pak and/or devtools_resources.pak) directory.
    if (!config.resourcesDirPath.empty()) {
        CefString(&settings.resources_dir_path) = CefString(config.resourcesDirPath);
    }

    // The locales directory.
    if (!config.localesDirPath.empty()) {
        CefString(&settings.locales_dir_path) = CefString(config.localesDirPath);
    }

    // Enable remote debugging on the specified port.
    settings.remote_debugging_port = config.remoteDebuggingPort;
    settings.no_sandbox = config.noSandbox;

#ifndef SUB_PROCESS_DISABLED
    std::string subProcessPath = PathUtil::GetAppWorkingDirectory() + "/browser.exe";
    CefString(&settings.browser_subprocess_path) = CefString(subProcessPath);
#endif

    return settings;
}

#if defined(WIN32)
    std::string CefContext::GetProcessType() {
        CefRefPtr<CefCommandLine> commandLine = CefCommandLine::CreateCommandLine();
        commandLine->InitFromString(::GetCommandLine());
#else
    std::string CefContext::GetProcessType(int argc, char* argv[]) {
        CefRefPtr<CefCommandLine> commandLine = CefCommandLine::CreateCommandLine();
        commandLine->InitFromArgv(argc, argv);
#endif
        if (!commandLine->HasSwitch("type")) {
            return "browser";
        }

        return commandLine->GetSwitchValue("type").ToString();
    }


CefContext::CefContext(const CefConfig& config)
    : _config(config) {
}

CefContext::~CefContext() {
    CefShutdown();
}

#if defined(WIN32)
int CefContext::initialize(std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate,
                           std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate) {
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    CefMainArgs mainArgs(hInstance);
    std::string processType = GetProcessType();
#else
int CefContext::initialize(int argc,
                           char* argv[],
                           std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate,
                           std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate) {
    CefMainArgs mainArgs(argc, argv);
    std::string processType = GetProcessType(argc, argv);
#endif

    // Create CefViewApp and register appropriate delegate based on process type
    CefRefPtr<CefViewApp> app;
    if (processType == "browser") {
        app = new CefViewApp(browserAppDelegate);
    } else if (processType == "renderer") {
        app = new CefViewApp(rendererAppDelegate);
    } else {
        // Other process types (GPU, plugin, etc.) - no delegate
        app = new CefViewApp();
    }

#ifdef SUB_PROCESS_DISABLED
    // Execute sub-process logic if this is a sub-process.
    int exitCode = CefExecuteProcess(mainArgs, app.get(), NULL);
    if (exitCode >= 0) {
        // This is a sub-process, return exit code to terminate
        return exitCode;
    }
#endif

    // This is the browser process, initialize CEF
    CefSettings settings = getCefSettings(_config);
    void* sandboxInfo = NULL;

    if (!CefInitialize(mainArgs, settings, app.get(), sandboxInfo)) {
        return -2;  // Initialization failed
    }

    return -1;  // Browser process, continue execution
}

void CefContext::doMessageLoopWork() {
    if (_config.multiThreadedMessageLoop) {
        return;
    }
    CefDoMessageLoopWork();
}

}  // namespace cefview