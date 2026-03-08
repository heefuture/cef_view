#include "CefContext.h"
#include <client/CefViewApp.h>
#include <client/CefViewAppDelegateInterface.h>
#include <utils/PathUtil.h>
#include <utils/LogUtil.h>

#include "include/cef_app.h"
#include "include/cef_scheme.h"

#include <string>

// On Windows, disable separate sub-process executable.
// The main process handles all CEF functionality in single-process mode.
// On macOS, Helper app bundles handle sub-processes independently.
#if defined(WIN32)
#define SUB_PROCESS_DISABLED
#endif

namespace cefview {

static CefSettings getCefSettings(const CefConfig& config) {
    CefSettings settings;
    settings.multi_threaded_message_loop = config.multiThreadedMessageLoop;
    settings.log_severity = LOGSEVERITY_WARNING;

    CefString(&settings.cookieable_schemes_list) = config.cookieableSchemesList;

    // Resolve base directory for cache and log.
    // macOS app bundle working directory is not writable, use system cache directory.
    // Windows uses the app working directory.
    std::string baseDir = "";
    if (config.cachePath.empty()) {
#if defined(__APPLE__)
        baseDir = PathUtil::GetAppCacheDirectory("");
#else
        baseDir = PathUtil::GetAppWorkingDirectory();
#endif
    }

    // Store cache data on disk.
    if (config.cachePath.empty()) {
        std::string cachePath = baseDir + PathUtil::sPathSep + "cache";
        CefString(&settings.cache_path) = CefString(cachePath);
    } else {
        CefString(&settings.cache_path) = CefString(config.cachePath);
    }

    // Set debug log file path.
    if (config.logFilePath.empty()) {
        std::string logDir = baseDir.empty() ? config.cachePath : baseDir;
        std::string logPath = logDir + PathUtil::sPathSep + "cef.log";
        CefString(&settings.log_file) = CefString(logPath);
    } else {
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

    // Set locale from config
    if (!config.locale.empty()) {
        CefString(&settings.locale) = CefString(config.locale);
    }

    // Enable remote debugging on the specified port.
    settings.remote_debugging_port = config.remoteDebuggingPort;
    settings.no_sandbox = config.noSandbox;

    // Enable windowless (off-screen) rendering support.
    // This must be enabled globally for any browser to use OSR mode.
    settings.windowless_rendering_enabled = config.windowlessRenderingEnabled;

    // Set background color from config
    settings.background_color = config.backgroundColor;

#if defined(WIN32) && !defined(SUB_PROCESS_DISABLED)
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
                           std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate,
                           std::shared_ptr<CefViewAppDelegateInterface> schemeAppDelegate) {
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    CefMainArgs mainArgs(hInstance);
    std::string processType = GetProcessType();
#else
int CefContext::initialize(int argc,
                           char* argv[],
                           std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate,
                           std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate,
                           std::shared_ptr<CefViewAppDelegateInterface> schemeAppDelegate) {
    CefMainArgs mainArgs(argc, argv);
    std::string processType = GetProcessType(argc, argv);
#endif

    // Create CefViewApp and register appropriate delegate based on process type
    CefRefPtr<CefViewApp> app;
    if (processType == "browser") {
        app = new CefViewApp(_config, browserAppDelegate);
    } else if (processType == "renderer") {
        app = new CefViewApp(_config, rendererAppDelegate);
    } else {
        // Other process types (GPU, plugin, etc.) - no delegate
        app = new CefViewApp(_config);
    }

    // Register scheme delegate for all processes if provided
    if (schemeAppDelegate) {
        app->RegisterDelegate(schemeAppDelegate);
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
        LOGE << "CefInitialize failed!";
        return -2;  // Initialization failed
    }

    LOGI << "CefView initialized successfully";

    return -1;  // Browser process, continue execution
}

void CefContext::runMessageLoop() {
    if (_config.multiThreadedMessageLoop) {
        return;
    }
    CefRunMessageLoop();
}

void CefContext::quitMessageLoop() {
    if (_config.multiThreadedMessageLoop) {
        return;
    }
    CefQuitMessageLoop();
}

void CefContext::doMessageLoopWork() {
    if (_config.multiThreadedMessageLoop) {
        return;
    }
    CefDoMessageLoopWork();
}

bool CefContext::RegisterSchemeHandlerFactory(const std::string& schemeName,
                                               const std::string& domainName,
                                               CefRefPtr<CefSchemeHandlerFactory> factory) {
    return CefRegisterSchemeHandlerFactory(schemeName, domainName, factory);
}

}  // namespace cefview
