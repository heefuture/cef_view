# Quick Start: CefWebView Control

**Feature**: 001-cefwebview-control  
**Date**: 2025-11-18  
**Audience**: C++开发者，熟悉Windows编程和CEF基础概念

## Prerequisites

在开始之前，确保以下条件满足：

- ✅ Windows 10/11 64位系统
- ✅ Visual Studio 2022 (MSVC编译器)
- ✅ CMake 3.14+
- ✅ CEF二进制文件已集成到项目（`libcef/`目录）
- ✅ Direct3D 11运行时支持（Windows 10自带）

## Build & Run

### 1. 配置并构建项目

```powershell
# 在项目根目录执行
cmake -G "Visual Studio 17 2022" -A x64 -S . -B ./build_vc -DUSE_SANDBOX=OFF -DCMAKE_BUILD_TYPE=Debug

# 打开解决方案
start build_vc/cefApp.sln

# 或使用命令行构建
cmake --build ./build_vc --config Debug
```

### 2. 运行演示应用

```powershell
# 确保CEF运行时文件在输出目录
# 运行主应用程序
./build_vc/Debug/cefApp.exe
```

**预期结果**:
- 打开一个主窗口，顶部50%区域显示一个网页（百度首页）
- 底部50%区域显示3个叠加的网页（Google/GitHub/Bing），可通过按钮切换

---

## Usage Examples

### Example 1: 创建单个窗口模式浏览器

```cpp
#include <cefview/view/CefWebView.h>
#include <cefview/manager/CefManager.h>

// 在WinMain或主窗口创建时

int APIENTRY wWinMain(HINSTANCE hInstance, ...) {
  // 1. 初始化CEF
  CefSettings cefSettings;
  CefManager::getInstance()->initCef(cefSettings, true);

  // 2. 创建主窗口
  HWND mainHwnd = CreateWindowEx(...);

  // 3. 配置CefWebView
  using namespace cefview;
  CefWebViewSetting settings = CefWebViewSetting()
    .setRenderMode(RenderMode::Window)      // 窗口模式
    .setInitialSize(800, 600)
    .setBackgroundColor(0xFFFFFFFF);

  // 4. 创建浏览器控件
  auto webView = std::make_shared<CefWebView>(
    "https://www.baidu.com",  // 初始URL
    settings,
    mainHwnd
  );

  // 5. 初始化（异步）
  webView->init();

  // 6. 后续操作（可立即调用，会自动排队）
  webView->setRect(0, 0, 800, 600);

  // 7. 运行消息循环
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    
    // CEF消息循环（必须）
    CefManager::getInstance()->doCefMessageLoopWork();
  }

  // 8. 清理
  webView.reset();  // 释放CefWebView
  CefManager::getInstance()->quitCef();

  return 0;
}
```

---

### Example 2: 创建离屏渲染浏览器（D3D11硬件加速）

```cpp
// 在MainWindow::onCreate中

void MainWindow::createOffScreenView() {
  using namespace cefview;
  
  // 配置离屏渲染模式
  CefWebViewSetting settings = CefWebViewSetting()
    .setRenderMode(RenderMode::OffScreen)   // 离屏模式
    .setInitialSize(1920, 1080)
    .setBackgroundColor(0xFF000000)         // 黑色背景
    .enableGPUAcceleration = true;          // 启用GPU加速

  // 创建离屏浏览器
  _offScreenView = std::make_shared<CefWebView>(
    "https://webglsamples.org/aquarium/aquarium.html",  // WebGL示例
    settings,
    _hwnd
  );

  _offScreenView->init();
  _offScreenView->setRect(0, 0, 1920, 1080);
}

// WM_SIZE处理（窗口大小变化）
void MainWindow::onSize(int width, int height) {
  if (_offScreenView) {
    _offScreenView->setRect(0, 0, width, height);
    // D3D11Renderer会自动调整交换链和纹理大小
  }
}

// WM_PAINT处理（离屏模式）
void MainWindow::onPaint() {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(_hwnd, &ps);
  
  // 离屏模式：D3D11Renderer已在CEF的OnPaint回调中Present
  // 这里仅需ValidateRect标记已重绘
  ValidateRect(_hwnd, nullptr);
  
  EndPaint(_hwnd, &ps);
}
```

**关键点**:
- 离屏模式下，CEF渲染内容通过`CefRenderHandler::OnPaint`回调传递
- `D3D11Renderer`接收BGRA buffer，更新到纹理，并Present到窗口
- 窗口无需手动绘制，D3D11交换链自动呈现

---

### Example 3: 管理多个叠加的离屏视图

```cpp
class MainWindow {
private:
  std::shared_ptr<CefWebView> _topView;               // 顶部单视图
  std::list<std::shared_ptr<CefWebView>> _bottomViews; // 底部叠加视图
  int _activeBottomIndex = 0;

public:
  void createMultiViews() {
    using namespace cefview;

    // 顶部视图（窗口模式）
    CefWebViewSetting topSettings = CefWebViewSetting()
      .setRenderMode(RenderMode::Window)
      .setInitialSize(1920, 540);
    
    _topView = std::make_shared<CefWebView>("https://www.baidu.com", topSettings, _hwnd);
    _topView->init();
    _topView->setRect(0, 0, 1920, 540);

    // 底部3个叠加视图（离屏模式）
    std::vector<std::string> urls = {
      "https://www.google.com",
      "https://github.com",
      "https://www.bing.com"
    };

    CefWebViewSetting bottomSettings = CefWebViewSetting()
      .setRenderMode(RenderMode::OffScreen)
      .setInitialSize(1920, 540);

    for (const auto& url : urls) {
      auto view = std::make_shared<CefWebView>(url, bottomSettings, _hwnd);
      view->init();
      view->setRect(0, 540, 1920, 540);  // 所有视图相同位置
      _bottomViews.push_back(view);
    }
  }

  // 切换激活的底部视图
  void setActiveBottomView(int index) {
    if (index < 0 || index >= _bottomViews.size()) return;

    // 移动选中视图到列表末尾（绘制在最上层）
    auto it = _bottomViews.begin();
    std::advance(it, index);
    std::shared_ptr<CefWebView> activeView = *it;
    _bottomViews.erase(it);
    _bottomViews.push_back(activeView);

    _activeBottomIndex = index;

    // 强制重绘所有底部视图
    InvalidateRect(_hwnd, nullptr, FALSE);
  }

  // WM_COMMAND处理（按钮点击）
  void onCommand(UINT id) {
    switch (id) {
      case IDC_TAB1: setActiveBottomView(0); break;
      case IDC_TAB2: setActiveBottomView(1); break;
      case IDC_TAB3: setActiveBottomView(2); break;
    }
  }
};
```

**Z-Order管理**:
- 离屏模式下，通过绘制顺序控制Z轴顺序
- 最后绘制的视图显示在最上层
- 使用`std::list`便于元素移动，维护绘制顺序

---

### Example 4: 浏览器控制与JavaScript交互

```cpp
// 导航控制
webView->loadUrl("https://www.example.com");
webView->refresh();
webView->stopLoad();

// 查询状态
bool isLoading = webView->isLoading();
std::string currentUrl = webView->getUrl();

// 执行JavaScript
webView->evaluateJavaScript(R"(
  // 修改页面标题
  document.title = 'Modified by C++';
  
  // 添加元素
  var div = document.createElement('div');
  div.textContent = 'Hello from CefWebView!';
  div.style.cssText = 'font-size:32px; color:red; position:fixed; top:10px; left:10px;';
  document.body.appendChild(div);
)");

// 页面缩放
webView->setZoomLevel(1.0);  // 约110%
webView->setZoomLevel(-1.0); // 约90%

// 打开开发者工具
webView->openDevTools();

// 触发下载
webView->startDownload("https://example.com/file.pdf");
```

---

### Example 5: 事件处理（重写CefWebView）

```cpp
class MyWebView : public cefview::CefWebView {
public:
  using CefWebView::CefWebView;  // 继承构造函数

  // 重写事件回调
  void onLoadStart(const std::string& url) override {
    std::cout << "开始加载: " << url << std::endl;
    CefWebView::onLoadStart(url);  // 调用基类实现
  }

  void onLoadEnd(const std::string& url) override {
    std::cout << "加载完成: " << url << std::endl;
    
    // 页面加载完成后，自动注入JavaScript
    evaluateJavaScript("console.log('Page loaded by MyWebView');");
    
    CefWebView::onLoadEnd(url);
  }

  void onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl) override {
    std::cerr << "加载失败: " << failedUrl << " - " << errorText << std::endl;
    
    // 显示自定义错误页面
    std::string errorHtml = R"(<html><body><h1>加载失败</h1><p>)" 
      + errorText + R"(</p></body></html>)";
    // 注意：直接LoadHTML可能需要data: URL
    CefWebView::onLoadError(browserId, errorText, failedUrl);
  }

  void onTitleChange(int browserId, const std::string& title) override {
    std::cout << "标题变化: " << title << std::endl;
    
    // 更新主窗口标题
    SetWindowTextA(GetParent(getWindowHandle()), title.c_str());
    
    CefWebView::onTitleChange(browserId, title);
  }
};

// 使用自定义类
auto webView = std::make_shared<MyWebView>("https://www.example.com", settings, _hwnd);
webView->init();
```

---

## Best Practices

### 1. 资源管理

```cpp
// ✅ 正确：使用std::shared_ptr管理CefWebView
class MainWindow {
private:
  std::shared_ptr<CefWebView> _webView;

public:
  ~MainWindow() {
    // 显式释放（虽然智能指针会自动释放）
    _webView.reset();
  }
};

// ❌ 错误：使用裸指针
CefWebView* webView = new CefWebView(...);  // 容易内存泄漏
```

### 2. 异步初始化处理

```cpp
// ✅ 正确：init()后立即调用API（会自动排队）
webView->init();
webView->loadUrl("https://www.example.com");  // 加入任务队列
webView->setRect(0, 0, 800, 600);             // 加入任务队列

// ✅ 也可以在onAfterCreated回调中操作
class MyWebView : public CefWebView {
  void onAfterCreated(int browserId) override {
    CefWebView::onAfterCreated(browserId);
    
    // 此时浏览器已就绪，可以安全调用
    loadUrl("https://www.example.com");
  }
};
```

### 3. 线程安全

```cpp
// ✅ 正确：所有API调用在UI线程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  MainWindow* pThis = GetUserDataPtr<MainWindow*>(hwnd);
  
  switch (msg) {
    case WM_COMMAND:
      pThis->_webView->loadUrl("https://new-url.com");  // UI线程✓
      break;
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

// ❌ 错误：在工作线程调用
std::thread worker([]() {
  webView->loadUrl("...");  // 未定义行为！CEF API不是线程安全的
});
```

### 4. DPI感知

```cpp
// 在WinMain初始化
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

// 在WM_DPICHANGED处理
case WM_DPICHANGED: {
  UINT newDpi = HIWORD(wParam);
  float scaleFactor = newDpi / 96.0f;
  
  // 更新所有CefWebView的缩放
  for (auto& view : _allViews) {
    view->setDeviceScaleFactor(scaleFactor);
  }
  break;
}
```

### 5. 错误处理

```cpp
// 重写onLoadError处理加载失败
void MyWebView::onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl) {
  // 记录日志
  LogError("Load failed: " + failedUrl + " - " + errorText);
  
  // 显示友好的错误页面
  std::string errorPage = generateErrorPage(errorText, failedUrl);
  evaluateJavaScript("document.body.innerHTML = `" + errorPage + "`;");
}
```

---

## Common Pitfalls

### ❌ Pitfall 1: 忘记调用init()

```cpp
auto webView = std::make_shared<CefWebView>(...);
// 忘记调用init()
webView->loadUrl("...");  // 永远不会执行（浏览器未创建）
```

**Solution**: 构造后立即调用`init()`。

---

### ❌ Pitfall 2: 在浏览器创建前同步等待

```cpp
webView->init();
Sleep(1000);  // ❌ 错误：期望浏览器已创建
webView->loadUrl("...");
```

**Solution**: 不要等待，直接调用API（会自动排队）或重写`onAfterCreated`。

---

### ❌ Pitfall 3: setRect传入负数或0

```cpp
webView->setRect(0, 0, 0, 600);  // ❌ width=0会抛出异常
```

**Solution**: 始终确保width和height > 0。

---

### ❌ Pitfall 4: 手动调用事件回调

```cpp
webView->onLoadStart("...");  // ❌ 不应手动调用
```

**Solution**: 事件回调仅由`CefViewClientDelegate`调用，应用层通过重写虚函数接收事件。

---

### ❌ Pitfall 5: 循环引用导致内存泄漏

```cpp
class MyClass {
  std::shared_ptr<CefWebView> _webView;
  
  void setup() {
    _webView = std::make_shared<CefWebView>(...);
    
    // ❌ lambda捕获this导致循环引用
    _webView->setCallback([this]() {
      // this持有_webView，_webView通过lambda持有this
    });
  }
};
```

**Solution**: 使用`std::weak_ptr`或避免在CefWebView中存储引用外部对象的lambda。

---

## Troubleshooting

### 问题1: 浏览器窗口不显示

**可能原因**:
- 未调用`init()`
- 父窗口HWND无效
- setRect尺寸为0
- 窗口被其他窗口遮挡

**Solution**:
```cpp
// 检查父窗口
assert(IsWindow(parentHwnd));

// 确保初始化
webView->init();

// 设置合理尺寸
webView->setRect(0, 0, 800, 600);

// 显式显示
webView->setVisible(true);
```

---

### 问题2: 离屏渲染黑屏

**可能原因**:
- D3D11设备创建失败（不支持Feature Level 11.0）
- GPU驱动过旧
- CEF OnPaint未触发

**Solution**:
```cpp
// 检查D3D11支持
D3D_FEATURE_LEVEL featureLevel;
HRESULT hr = D3D11CreateDevice(..., &featureLevel, ...);
if (FAILED(hr) || featureLevel < D3D_FEATURE_LEVEL_11_0) {
  // 降级到窗口模式
}

// 检查OnPaint是否被调用（添加日志）
void CefViewClientDelegate::onPaint(...) {
  std::cout << "OnPaint called: " << width << "x" << height << std::endl;
  // ...
}
```

---

### 问题3: 鼠标点击无响应（离屏模式）

**可能原因**:
- 鼠标事件未转发给CEF
- 坐标转换错误

**Solution**:
```cpp
// 确保WndProc处理鼠标事件
case WM_LBUTTONDOWN: {
  if (_renderMode == RenderMode::OffScreen && _browser) {
    CefMouseEvent mouseEvent;
    mouseEvent.x = GET_X_LPARAM(lParam);
    mouseEvent.y = GET_Y_LPARAM(lParam);
    mouseEvent.modifiers = getCefMouseModifiers(wParam);
    
    _browser->GetHost()->SendMouseClickEvent(mouseEvent, MBT_LEFT, false, 1);
  }
  break;
}
```

---

## Next Steps

完成快速开始后，您可以：

1. **阅读API契约**: 查看`contracts/cefwebview-api.md`了解所有方法详细说明
2. **查看数据模型**: 阅读`data-model.md`理解内部架构和类关系
3. **运行演示应用**: 构建并运行`src/app/`中的完整示例
4. **自定义控件**: 继承`CefWebView`重写事件回调实现自定义逻辑
5. **集成到项目**: 将`cefview/`库集成到您的Windows应用程序

**参考资源**:
- CEF官方文档: https://bitbucket.org/chromiumembedded/cef/wiki/Home
- Direct3D 11文档: https://learn.microsoft.com/en-us/windows/win32/direct3d11
- 项目Constitution: `../.specify/memory/constitution.md`

---

**祝您使用愉快！如有问题，请参考`research.md`中的技术决策和最佳实践。**
