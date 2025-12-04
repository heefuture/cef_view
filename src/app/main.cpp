#include <windows.h>
#include <string>

#include "MainWindow.h"
#include <global/CefContext.h>
#include <client/CefViewAppDelegateRenderer.h>

using namespace cefview;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Create delegates for browser and renderer processes
    std::shared_ptr<CefViewAppDelegateInterface> browserDelegate = nullptr;  // Can be set if needed
    std::shared_ptr<CefViewAppDelegateInterface> rendererDelegate = std::make_shared<CefViewAppDelegateRenderer>();

    // Initialize CEF context
    cefview::CefConfig cefConfig;
    cefview::CefContext context(cefConfig);

    int initResult = context.initialize(browserDelegate, rendererDelegate);
    if (initResult >= 0) {
        // This is a sub-process, exit immediately with the returned code
        return initResult;
    }

#ifdef _DEBUG
    // Allocate a console for std::cout output in debug mode
    AllocConsole();
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCout, "CONOUT$", "w", stderr);
#endif

    auto mainWindow = std::make_unique<MainWindow>();

    auto config = std::make_unique<MainWindow::Config>();
    config->showMode = MainWindow::ShowMode::NORMAL;
    config->bounds = {0, 0, 1280, 800};  // 默认窗口大小
    config->initiallyHidden = false;

    mainWindow->init(std::move(config));

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        context.doMessageLoopWork();
    }

    return static_cast<int>(msg.wParam);
}