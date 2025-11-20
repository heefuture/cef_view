#include <windows.h>
#include <string>

#include "MainWindow.h"
#include <global/CefContext.h>

using namespace cefview;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CefConfig cefConfig;
    CefContext context(cefConfig);

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