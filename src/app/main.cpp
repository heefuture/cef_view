#include <windows.h>

#include <manager/CefManager.h>

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"MyWindowClass";

     CefSettings cefSettings;
    int result = CefManager::getInstance()->initCef(cefSettings, false);
    if (result == 0)
        return result;

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // 扩展样式
        CLASS_NAME,                     // 窗口类名
        L"空窗体",                      // 窗口标题
        WS_OVERLAPPEDWINDOW,            // 窗口样式
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, // 位置和大小
        nullptr, nullptr, hInstance, nullptr
    );

    if (hwnd == nullptr) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);


    if (!cefSettings.multi_threaded_message_loop) {
        CefManager::getInstance()->doCefMessageLoopWork();
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    CefManager::getInstance()->quitCef();
    return 0;
}