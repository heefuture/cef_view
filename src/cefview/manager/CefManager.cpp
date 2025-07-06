#include "CefManager.h"

#include <string.h>
#include "CefManager.h"
//#include "cefclient/client_switches.h"

// Whether to use a separate sub-process executable? cefclient_process.exe
//#define SUB_PROCESS_DISABLED
namespace cef{

static CefManager* CefManager::getInstance() {
    static CefManager typeInstance;
    return &typeInstance;
}

// Initialize the CEF settings.
void CefManager::initCefSettings(CefSettings& settings) {

    // 调试的时候开启开启单进程模式
    // settings.single_process = false;
    // Make browser process message loop run in a separate thread.
    // Macos is not supported, so we need to run messageloop
    settings.multi_threaded_message_loop = true;

    std::string list_str("http,https");
    CefString(&settings.cookieable_schemes_list) = list_str;

    // Store cache data will on disk.
    std::string cache_path = getAppWorkingDirectory().ToString() + "/.cache";
    CefString(&settings.cache_path) = CefString(cache_path);

    // Completely disable logging.
    settings.log_severity = LOGSEVERITY_DISABLE;
    // 设置debug log文件位置
    //std::string log_path = getAppWorkingDirectory().ToString() + "/cef.log";
    //CefString(&settings.log_file) = CefString(log_path);
    // The resources(cef.pak and/or devtools_resources.pak) directory.
    CefString(&settings.resources_dir_path) = CefString();
    // The locales directory.
    CefString(&settings.locales_dir_path) = CefString();
    // Enable remote debugging on the specified port.
    settings.remote_debugging_port = 18432;
    // pack_loading_disabled
    // settings.pack_loading_disabled=true;
    // Ignore errors related to invalid SSL certificates.
    // settings.ignore_certificate_errors = true;
    settings.no_sandbox = true;
#ifndef SUB_PROCESS_DISABLED
    std::string sub_process_path = getAppWorkingDirectory().ToString() + "/browser.exe";
    CefString(&settings.browser_subprocess_path) = CefString(sub_process_path);
    // LOG_MSG_FILE("sub_process_path = %s",sub_process_path.c_str());
#endif

    //g_command_line->AppendSwitch(cefclient::kOffScreenRenderingEnabled);
    //settings.windowless_rendering_enabled = true;
}

int CefManager::initCef(CefSettings &settings, bool logging)
{
    int init_code = 0;
    // if (logging)
    // {
    //     OPEN_LOG = true;
    //     TCHAR tempPath[MAX_PATH];
    //     DWORD hr = GetTempPath(MAX_PATH, tempPath);
    //     if (hr != 0)
    //     {
    //         CefString tp(tempPath);
    //         auto nowt = time(NULL);
    //         tm nowtm;
    //         localtime_s(&nowtm, &nowt);
    //         wchar_t buf[16];
    //         wcsftime(buf, sizeof(buf), L"%Y-%m-%d", &nowtm);
    //         std::wstring logfile = tp.ToWString().append(L"CEF_").append(buf).append(L".log");
    //         std::string logfile_ascii = unicode_to_ascii(logfile.c_str());
    //         setOutputFileName(logfile_ascii.c_str());
    //     }
    // }
#if defined(OS_WIN)
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    CefMainArgs main_args(hInstance);
#elif defined(OS_MACOSX)
    CefMainArgs main_args();
#endif
    CefRefPtr<ClientApp> app(new ClientApp);

#ifdef SUB_PROCESS_DISABLED
    // Execute the secondary process, if any.
    int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
    if (exit_code >= 0)
        return init_code;
#endif

    CefSettings settings;
    initCefSettings(settings);

    void *sandbox_info = NULL;
    // Initialize CEF.
    init_code = CefInitialize(main_args, settings, app.get(), sandbox_info);
    return init_code;
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
#if defined(OS_WIN)
    std::unique_ptr<TCHAR[]> buffer(new TCHAR[MAX_PATH + 1]);
    if (!::GetTempPath(MAX_PATH, buffer.get()))
        return CefString();
    else
        return CefString(buffer.get());
#elif defined(OS_MACOSX)
    return CefString();
#elif defined(USE_QT)
  return CefString(reinterpret_cast<const WCHAR*>(QDir::tempPath().utf16()));
#else
    return CefString();
#endif
}


void CefManager::quitCef()
{
    CefShutdown();
}

void CefManager::doCefMessageLoopWork()
{
    CefDoMessageLoopWork();
}

void CefManager::quitCefMessageLoop()
{
    CefQuitMessageLoop();
}

}