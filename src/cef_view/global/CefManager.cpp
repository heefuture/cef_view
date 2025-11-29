#include "CefManager.h"
#include <client/CefViewApp.h>
#include <utils/PathUtil.h>
#include <string>

//#include "cefclient/client_switches.h"

// Whether to use a separate sub-process executable? cefclient_process.exe
//
#define SUB_PROCESS_DISABLED

namespace cefview {

CefManager* CefManager::getInstance() {
    static CefManager typeInstance;
    return &typeInstance;
}

CefSettings CefManager::initCefSettings(const CefConfig& config) {
    CefSettings settings;
    settings.multi_threaded_message_loop = config.multiThreadedMessageLoop;
    CefString(&settings.cookieable_schemes_list) = config.cookieableSchemesList;

    // Store cache data will on disk.
    if (config.cachePath.empty()) {
        std::string cachePath = getAppWorkingDirectory().ToString() + "/cache";
        CefString(&settings.cache_path) = CefString(cachePath);
    } else {
        CefString(&settings.cache_path) = CefString(config.cachePath);
    }

    // Completely disable logging.
    // settings.log_severity = LOGSEVERITY_DISABLE
    if (config.logFilePath.empty()) {
        // Set debug log file path
        std::string log_path = getAppWorkingDirectory().ToString() + "/cef.log";
        CefString(&settings.log_file) = CefString(log_path);
    } else {
        CefString(&settings.log_file) = CefString(config.logFilePath);
    }
    // settings.log_severity = config.logSeverity;

    // The resources(cef.pak and/or devtools_resources.pak) directory.
    if (config.resourcesDirPath.empty()) {
        CefString(&settings.resources_dir_path) = CefString();
    } else {
        CefString(&settings.resources_dir_path) = CefString(config.resourcesDirPath);
    }

    // The locales directory.
    if (config.localesDirPath.empty()) {
        CefString(&settings.locales_dir_path) = CefString();
    } else {
        CefString(&settings.locales_dir_path) = CefString(config.localesDirPath);
    }

    // Enable remote debugging on the specified port.
    settings.remote_debugging_port = config.remoteDebuggingPort;
    // pack_loading_disabled
    // settings.pack_loading_disabled=true;
    // Ignore errors related to invalid SSL certificates.
    // settings.ignore_certificate_errors = true;
    settings.no_sandbox = config.noSandbox;

#ifndef SUB_PROCESS_DISABLED
    std::string sub_process_path = getAppWorkingDirectory().ToString() + "/browser.exe";
    CefString(&settings.browser_subprocess_path) = CefString(sub_process_path);
    // LOG_MSG_FILE("sub_process_path = %s",sub_process_path.c_str());
#endif

    //g_command_line->AppendSwitch(cefclient::kOffScreenRenderingEnabled);
    //settings.windowless_rendering_enabled = true;
    return settings;
}

int CefManager::initCef(const CefConfig& config)
{
    int initCode = 0;
#if defined(OS_WIN)
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    CefMainArgs main_args(hInstance);
#elif defined(OS_MACOSX)
    CefMainArgs main_args();
#endif
    CefRefPtr<CefViewApp> app(new CefViewApp);

    CefRefPtr<CefCommandLine> commandLine = CefCommandLine::CreateCommandLine();
#if defined(OS_WIN)
    commandLine->InitFromString(::GetCommandLine());
#else
    // commandLine->InitFromArgv(argc, argv);
#endif



#ifdef SUB_PROCESS_DISABLED
    // Execute the secondary process, if any.
    int exitCode = CefExecuteProcess(main_args, app.get(), NULL);
    if (exitCode >= 0)
        return initCode;
#endif

    CefSettings settings = initCefSettings(config);
    void *sandbox_info = NULL;
    // Initialize CEF.
    initCode = CefInitialize(main_args, settings, app.get(), sandbox_info);
    return initCode;
}

CefString CefManager::getAppWorkingDirectory() {
#if defined(OS_WIN)
    std::unique_ptr<TCHAR[]> buffer(new TCHAR[MAX_PATH + 1]);
    if (!::GetCurrentDirectory(MAX_PATH, buffer.get()))
        return CefString();
    else
        return CefString(buffer.get());
#elif defined(OS_MACOSX)
    return CefString();
#elif defined(USE_QT)
    return CefString(reinterpret_cast<const WCHAR*>(QCoreApplication::applicationDirPath().utf16()));
#else
    return CefString();
#endif
}

CefString CefManager::getAppTempDirectory()
{
    std::string tempDir = cefview::PathUtil::GetAppTempDirectory();
    return CefString(tempDir);
}


void CefManager::quitCef()
{
    CefShutdown();
}

void CefManager::doCefMessageLoopWork()
{
    CefDoMessageLoopWork();
}

void CefManager::runCefMessageLoop()
{
    CefRunMessageLoop();
}

void CefManager::quitCefMessageLoop()
{
    CefQuitMessageLoop();
}

}