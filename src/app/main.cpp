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
    cefConfig.backgroundColor = 0x00000000;  // Fully transparent background

    // Follow system language settings
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0) {
        int size = WideCharToMultiByte(CP_UTF8, 0, localeName, -1, nullptr, 0, nullptr, nullptr);
        if (size > 0) {
            std::string langStr(size - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, localeName, -1, &langStr[0], size, nullptr, nullptr);
            cefConfig.locale = langStr;
            // Set Accept-Language, add English as fallback for non-English locales
            cefConfig.acceptLanguageList = langStr;
            if (langStr.find("en") != 0) {
                cefConfig.acceptLanguageList += ",en-US;q=0.9,en;q=0.8";
            }
        }
    }

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