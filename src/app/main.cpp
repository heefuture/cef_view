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

    const int windowWidth = 1920;
    const int windowHeight = 1080;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    auto config = std::make_unique<MainWindow::Config>();
    config->showMode = MainWindow::ShowMode::NORMAL;
    config->bounds = {posX, posY, posX + windowWidth, posY + windowHeight};

    auto mainWindow = std::make_unique<MainWindow>();
    mainWindow->init(std::move(config));

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        context.doMessageLoopWork();
    }

    return static_cast<int>(msg.wParam);
}