/**
* @file        CefContext.h
* @brief       CEF context for initialization and lifecycle management
* @version     1.0
* @author      heefuture
* @date        2025.11.19
* @copyright
*/
#ifndef CEFCONTEXT_H
#define CEFCONTEXT_H
#pragma once

#include <global/CefConfig.h>
#include <memory>
#include <string>

namespace cefview {

class CefViewAppDelegateInterface;

class CefContext {
public:
    explicit CefContext(const CefConfig& config);
    ~CefContext();

    /**
     * Initialize CEF with optional delegates for browser and renderer processes.
     * @param browserAppDelegate Delegate for browser process (optional, can be nullptr)
     * @param rendererAppDelegate Delegate for renderer process (optional, can be nullptr)
     * @return Process execution result:
     *         -1: Browser process (main process), continue execution
     *         >= 0: Sub-process completed, should exit immediately with this code
     */
#if defined(WIN32)
    int initialize(std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate = nullptr,
                   std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate = nullptr);
#else
    int initialize(int argc,
                   char* argv[],
                   std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate = nullptr,
                   std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate = nullptr);
#endif

    /**
     * Run the CEF message loop. This will block until quitMessageLoop() is called.
     * Use this instead of doMessageLoopWork() for proper IME support.
     */
    void runMessageLoop();

    /**
     * Quit the CEF message loop that was started by runMessageLoop().
     */
    void quitMessageLoop();

    /**
     * Process CEF message loop work. Only call in single-threaded mode.
     */
    void doMessageLoopWork();

    /**
     * Get the CEF configuration.
     */
    const CefConfig& getCefConfig() const { return _config; }

    /**
     * Get the current process type from command line arguments.
     * @param argc Argument count (used on non-Windows platforms)
     * @param argv Argument values (used on non-Windows platforms)
     * @return Process type string: "browser" for browser process, "renderer", "gpu", etc. for sub-processes
     */
#if defined(WIN32)
    static std::string GetProcessType();
#else
    static std::string GetProcessType(int argc, char* argv[]);
#endif

private:
    CefConfig _config;
};

}  // namespace cefview

#endif  //!CEFCONTEXT_H