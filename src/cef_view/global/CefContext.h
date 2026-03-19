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

#include <memory>
#include <string>

#include <global/CefConfig.h>
#include <include/cef_scheme.h>

namespace cefview {

class CefViewAppDelegateInterface;

/**
 * Singleton that owns the CEF lifecycle (init / message-loop / shutdown).
 * Access via CefContext::instance(). Call initialize() before using other methods.
 */
class CefContext {
public:
    static CefContext& instance();

    CefContext(const CefContext&) = delete;
    CefContext& operator=(const CefContext&) = delete;
    CefContext(CefContext&&) = delete;
    CefContext& operator=(CefContext&&) = delete;

    /**
     * Initialize CEF with optional delegates for browser, renderer processes and scheme registration.
     * @param browserAppDelegate Delegate for browser process (optional, can be nullptr)
     * @param rendererAppDelegate Delegate for renderer process (optional, can be nullptr)
     * @param schemeAppDelegate Delegate for scheme registration (optional, can be nullptr)
     * @return Process execution result:
     *         -1: Browser process (main process), continue execution
     *         >= 0: Sub-process completed, should exit immediately with this code
     */
#if defined(WIN32)
    int initialize(const CefConfig& config,
                   std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate = nullptr,
                   std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate = nullptr,
                   std::shared_ptr<CefViewAppDelegateInterface> schemeAppDelegate = nullptr);
#else
    int initialize(int argc,
                   char* argv[],
                   const CefConfig& config,
                   std::shared_ptr<CefViewAppDelegateInterface> browserAppDelegate = nullptr,
                   std::shared_ptr<CefViewAppDelegateInterface> rendererAppDelegate = nullptr,
                   std::shared_ptr<CefViewAppDelegateInterface> schemeAppDelegate = nullptr);
#endif

    /**
     * Register a custom scheme handler factory.
     * Must be called after initialize() and only in browser process.
     * @param schemeName The scheme name (e.g., "cocraft")
     * @param domainName The domain name (e.g., "localhost"), empty for all domains
     * @param factory The scheme handler factory
     * @return true if registration succeeded
     */
    static bool RegisterSchemeHandlerFactory(const std::string& schemeName,
                                             const std::string& domainName,
                                             CefRefPtr<CefSchemeHandlerFactory> factory);

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
     * Explicitly shut down CEF. Must be called on the main thread after
     * runMessageLoop() returns.
     */
    void shutdown();

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
    CefContext();
    ~CefContext();

    static CefContext* sInstance;

    CefConfig _config;
    bool _messageLoopRunning = {false};
};

}  // namespace cefview

#endif  //!CEFCONTEXT_H