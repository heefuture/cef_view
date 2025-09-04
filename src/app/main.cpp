#include <windows.h>
#include <string>

#include <view/CefWebView.h>
#include <view/CefWebViewBase.h>
#include <manager/CefManager.h>

using namespace cefview;

// typedef void (*SubWindowRepaintCallback)(void*);

// struct SubWindowUserData {
//     SubWindowRepaintCallback repaint_callback;
//     void* repaint_callback_param;
// };

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
            // 调整底部子窗口高度
            //if (hwndBottom) {
            //int width = LOWORD(lParam);
            //int height = HIWORD(lParam);
                //SetWindowPos(hwndBottom, NULL, 0, 100, 1920, height, SWP_NOZORDER);
            //}
//            auto user_data =
//             (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
//         if (user_data && user_data->repaint_callback) {
//             user_data->repaint_callback(user_data->repaint_callback_param);
//         }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


// static LRESULT CALLBACK subWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//     if (uMsg == WM_PAINT) {
//         auto user_data =
//             (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
//         if (user_data && user_data->repaint_callback) {
//             user_data->repaint_callback(user_data->repaint_callback_param);
//         }
//     } else if (uMsg == WM_NCDESTROY) {
//         SubWindowUserData* user_data =
//             (SubWindowUserData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
//         delete user_data;
//     }
//     return DefWindowProc(hwnd, uMsg, wParam, lParam);
// }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const std::wstring ClassName = L"MainEmptyWindowClass";

    CefSettings cefSettings;
    int result = CefManager::getInstance()->initCef(cefSettings, false);
    if (result == 0)
        return result;

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = ClassName.c_str();
    ::RegisterClass(&wc);

    HWND hwnd = ::CreateWindowEx(
        0,                              // 扩展样式
        ClassName.c_str(),                     // 窗口类名
        L"CefApp",                      // 窗口标题
        WS_OVERLAPPEDWINDOW,            // 窗口样式
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 720, // 位置和大小
        nullptr, nullptr, hInstance, nullptr
    );

    if (hwnd == nullptr) {
        return 0;
    }

    ::ShowWindow(hwnd, nCmdShow);

    CefWebView* cefView = new CefWebView(hwnd);
    cefView->setVisible(true);
    cefView->loadURL("https://www.bing.com");
    //CefView.createSubWindow(hwnd, 0, 0, 800, 600, true);
    //CefView.setRect(0, 0, 800, 600); // 设置

    ::UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!cefSettings.multi_threaded_message_loop) {
            CefManager::getInstance()->doCefMessageLoopWork();
        }
    }
    // CefManager::getInstance()->runCefMessageLoop();

    CefManager::getInstance()->quitCef();
    return 0;
}