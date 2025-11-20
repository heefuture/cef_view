# Data Model: CefWebView Control with Dual Rendering Modes

**Feature**: 001-cefwebview-control  
**Date**: 2025-11-18  
**Phase**: 1 - Data Structures & Entity Design

## Overview

本文档定义CefWebView控件的核心数据结构、类关系和状态管理模型。所有实体设计遵循C++17标准和项目constitution定义的OOP原则。

## Core Entities

### 1. CefWebView (Primary Control Class)

**Purpose**: 浏览器控件的主类，封装CEF浏览器实例，管理渲染、事件处理和生命周期。

**File Location**: `src/cefview/view/CefWebView.h` (已存在，需完善)

**Class Definition**:

```cpp
namespace cefview {

enum class RenderMode {
  Window,    // 窗口渲染模式（CEF创建子HWND）
  OffScreen  // 离屏渲染模式（使用D3D11）
};

class CefWebView : public std::enable_shared_from_this<CefWebView> {
public:
  // 构造/析构
  CefWebView(const std::string& url, 
             const CefWebViewSetting& settings, 
             HWND parentHwnd);
  ~CefWebView();

  // 禁用拷贝和移动（控件不可复制）
  CefWebView(const CefWebView&) = delete;
  CefWebView& operator=(const CefWebView&) = delete;
  CefWebView(CefWebView&&) = delete;
  CefWebView& operator=(CefWebView&&) = delete;

  // 初始化（异步创建CEF浏览器）
  void init();

  // 导航控制
  void loadUrl(const std::string& url);
  const std::string& getUrl() const { return _url; }
  void refresh();
  void stopLoad();
  bool isLoading() const;

  // 布局控制
  void setRect(int left, int top, int width, int height);
  void setVisible(bool visible);

  // 高级功能
  bool openDevTools();
  void closeDevTools();
  bool isDevToolsOpened() const { return _isDevToolsOpened; }
  void evaluateJavaScript(const std::string& script);
  void setZoomLevel(float zoomLevel);
  void startDownload(const std::string& url);

  // 获取窗口句柄
  CefWindowHandle getWindowHandle() const;

  // 事件回调（由CefViewClientDelegate调用）
  void onTitleChange(int browserId, const std::string& title);
  void onUrlChange(int browserId, const std::string& oldUrl, const std::string& url);
  void onLoadingStateChange(int browserId, bool isLoading, bool canGoBack, bool canGoForward);
  void onLoadStart(const std::string& url);
  void onLoadEnd(const std::string& url);
  void onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl);
  void onAfterCreated(int browserId);
  void onBeforeClose(int browserId);
  void onProcessMessageReceived(int browserId, const std::string& messageName, const std::string& jsonArgs);

protected:
  // 内部实现
  HWND createSubWindow(HWND parentHwnd, int x, int y, int width, int height, bool showWindow = true);
  void createCefBrowser(const std::string& url, const CefWebViewSetting& settings);
  void destroy();

private:
  // 窗口句柄
  HWND _parentHwnd;                      // 父窗口句柄
  HWND _hwnd;                            // 自身窗口句柄（离屏模式下的容器窗口）
  std::string _className;                // 窗口类名

  // CEF对象
  CefRefPtr<CefBrowser> _browser;        // CEF浏览器实例
  CefRefPtr<CefViewClient> _client;      // CEF客户端
  std::shared_ptr<CefViewClientDelegate> _clientDelegate;  // 委托对象

  // 状态
  std::string _url;                      // 当前URL
  RenderMode _renderMode;                // 渲染模式
  bool _isDevToolsOpened;                // 开发者工具状态

  // 任务队列（用于浏览器创建前的延迟操作）
  typedef std::function<void(void)> StdClosure;
  std::vector<StdClosure> _taskListAfterCreated;

  // 离屏渲染器（仅离屏模式）
  std::unique_ptr<D3D11Renderer> _d3d11Renderer;
};

} // namespace cefview
```

**Key Attributes**:

| 属性 | 类型 | 说明 | 初始化 |
|------|------|------|--------|
| `_parentHwnd` | `HWND` | 父窗口句柄 | 构造函数传入 |
| `_hwnd` | `HWND` | 自身窗口句柄（离屏模式容器） | createSubWindow创建 |
| `_browser` | `CefRefPtr<CefBrowser>` | CEF浏览器实例 | createCefBrowser创建（异步） |
| `_client` | `CefRefPtr<CefViewClient>` | CEF客户端 | 构造时创建 |
| `_clientDelegate` | `std::shared_ptr<CefViewClientDelegate>` | 事件委托 | 构造时创建 |
| `_url` | `std::string` | 当前URL | 初始为空，loadUrl更新 |
| `_renderMode` | `RenderMode` | 渲染模式 | 从CefWebViewSetting读取 |
| `_d3d11Renderer` | `std::unique_ptr<D3D11Renderer>` | D3D11渲染器 | 离屏模式时创建 |
| `_taskListAfterCreated` | `std::vector<StdClosure>` | 任务队列 | 初始为空 |

**State Transitions**:

```
[Constructed] ─init()─> [Initializing] ─onAfterCreated()─> [Ready]
                                                               │
                                                               ├─ loadUrl() ─> [Loading] ─> [Ready]
                                                               ├─ setRect() ─> [Resizing] ─> [Ready]
                                                               └─ destroy() ─> [Destroyed]
```

**Validation Rules**:
- `init()`必须在构造后立即调用
- 所有导航/控制方法在`[Ready]`状态前加入任务队列
- URL必须是有效的HTTP/HTTPS协议（在loadUrl中验证）
- setRect的width和height必须>0

---

### 2. CefWebViewSetting (Configuration Object)

**Purpose**: 配置CefWebView的创建参数，采用Builder模式。

**File Location**: `src/cefview/view/CefWebViewSetting.h` (已存在)

**Class Definition**:

```cpp
namespace cefview {

struct CefWebViewSetting {
  // 渲染模式
  RenderMode renderMode = RenderMode::Window;

  // 浏览器设置
  bool enableJavaScript = true;
  bool enableLocalStorage = true;
  bool enableWebGL = true;
  bool enablePlugins = false;

  // 初始尺寸
  int initialWidth = 800;
  int initialHeight = 600;

  // 背景颜色（ARGB格式）
  uint32_t backgroundColor = 0xFFFFFFFF;  // 白色

  // GPU加速
  bool enableGPUAcceleration = true;

  // DPI缩放
  float deviceScaleFactor = 1.0f;

  // 离屏渲染特定配置
  struct OffScreenConfig {
    int frameRate = 60;  // 期望帧率
    bool enableVSync = true;
  } offScreenConfig;

  // Builder方法
  CefWebViewSetting& setRenderMode(RenderMode mode) {
    renderMode = mode;
    return *this;
  }

  CefWebViewSetting& setInitialSize(int width, int height) {
    initialWidth = width;
    initialHeight = height;
    return *this;
  }

  CefWebViewSetting& setBackgroundColor(uint32_t color) {
    backgroundColor = color;
    return *this;
  }

  // ... 其他Builder方法
};

} // namespace cefview
```

**Key Fields**:

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `renderMode` | `RenderMode` | `Window` | 渲染模式 |
| `enableJavaScript` | `bool` | `true` | 是否启用JS |
| `enableLocalStorage` | `bool` | `true` | 是否启用本地存储 |
| `initialWidth/Height` | `int` | `800x600` | 初始尺寸 |
| `backgroundColor` | `uint32_t` | `0xFFFFFFFF` | 背景色（ARGB） |
| `enableGPUAcceleration` | `bool` | `true` | GPU加速 |
| `deviceScaleFactor` | `float` | `1.0` | DPI缩放因子 |

**Usage Example**:

```cpp
CefWebViewSetting settings = CefWebViewSetting()
  .setRenderMode(RenderMode::OffScreen)
  .setInitialSize(1024, 768)
  .setBackgroundColor(0xFF000000);  // 黑色背景

auto webView = std::make_shared<CefWebView>("https://www.baidu.com", settings, parentHwnd);
webView->init();
```

---

### 3. CefViewClient (CEF Client Implementation)

**Purpose**: 实现CEF的CefClient接口及各Handler接口，聚合所有CEF事件。

**File Location**: `src/cefview/client/CefViewClient.h` (已存在)

**Class Definition** (简化，重点展示关键部分):

```cpp
namespace cefview {

class CefViewClient : public CefClient,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDownloadHandler,
                      public CefDragHandler,
                      public CefKeyboardHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRenderHandler,
                      public CefRequestHandler {
  IMPLEMENT_REFCOUNTING(CefViewClient);

public:
  CefViewClient(CefViewClientDelegateBase::RefPtr delegate);
  ~CefViewClient();

  // CefClient接口（返回各Handler）
  CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
  // ... 其他GetXxxHandler

  // CefRenderHandler接口（离屏渲染关键）
  bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
  void OnPaint(CefRefPtr<CefBrowser> browser,
               PaintElementType type,
               const RectList& dirtyRects,
               const void* buffer,
               int width, int height) override;

  // CefLifeSpanHandler接口
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  bool DoClose(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // CefLoadHandler接口
  void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override;
  void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
  void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

  // 其他Handler接口...

private:
  CefRefPtr<CefBrowser> _browser;              // 浏览器实例
  CefViewClientDelegateBase::RefPtr _delegate; // 委托对象
  bool _isClosing;                              // 关闭状态
  bool _isFocusOnEditableField;                 // 焦点在可编辑字段
};

} // namespace cefview
```

**Key Relationships**:
- CefViewClient聚合所有Handler接口（多接口继承）
- 通过`_delegate`将事件转发给CefViewClientDelegate
- 使用`IMPLEMENT_REFCOUNTING`宏实现CEF引用计数
- `_browser`保存浏览器实例引用

---

### 4. CefViewClientDelegate (Event Forwarding Layer)

**Purpose**: 将CEF事件转发给CefWebView，实现解耦。

**File Location**: `src/cefview/view/CefViewClientDelegate.h` (已存在)

**Class Definition**:

```cpp
namespace cefview {

class CefViewClientDelegate : public CefViewClientDelegateBase {
public:
  CefViewClientDelegate(const std::shared_ptr<CefWebView>& view);
  virtual ~CefViewClientDelegate();

  // 生命周期事件
  void onAfterCreated(CefRefPtr<CefBrowser> browser) override;
  void onBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // 加载事件
  void onLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override;
  void onLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
  void onLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

  // 显示事件
  void onTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
  void onAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override;

  // 渲染事件（离屏模式）
  bool onGetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
  void onPaint(CefRefPtr<CefBrowser> browser,
               PaintElementType type,
               const RectList& dirtyRects,
               const void* buffer,
               int width, int height) override;

private:
  std::weak_ptr<CefWebView> _view;  // 弱引用，避免循环引用
};

} // namespace cefview
```

**Ownership Model**:
```
CefWebView (std::shared_ptr)
  └─ owns ─> CefViewClientDelegate (std::shared_ptr)
               └─ weak ref ─> CefWebView (std::weak_ptr, 避免循环)
```

---

### 5. D3D11Renderer (Off-Screen Rendering Backend)

**Purpose**: 使用Direct3D 11硬件加速渲染CEF网页内容到窗口。

**File Location**: `src/cefview/osr_renderer/D3D11Renderer.h` (新增)

**Class Definition**:

```cpp
namespace cefview {

class D3D11Renderer {
public:
  D3D11Renderer(HWND hwnd, int width, int height);
  ~D3D11Renderer();

  // 禁用拷贝
  D3D11Renderer(const D3D11Renderer&) = delete;
  D3D11Renderer& operator=(const D3D11Renderer&) = delete;

  // 初始化/清理
  bool initialize();
  void uninitialize();

  // 窗口大小变化
  void resize(int width, int height);

  // 更新帧数据（从CEF OnPaint回调）
  void updateFrameData(PaintElementType type,
                       const RectList& dirtyRects,
                       const void* buffer,
                       int width, int height);

  // 渲染到窗口
  void render();

  // Popup支持
  void updatePopupVisibility(bool visible);
  void updatePopupRect(const CefRect& rect);

private:
  // 初始化函数
  bool createDeviceAndSwapchain();
  bool createShaderResource();
  bool createSampler();
  bool createBlender();
  bool createRenderTargetView();
  void setupPipeline();

  // 工具函数
  bool createQuadVertexBuffer(float x, float y, float w, float h, int viewWidth, int viewHeight, ID3D11Buffer** ppBuffer);
  void handleDeviceLost();

private:
  HWND _hwnd;
  int _width;
  int _height;

  // D3D11设备和上下文
  Microsoft::WRL::ComPtr<ID3D11Device> _d3dDevice;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> _d3dContext;

  // 交换链
  Microsoft::WRL::ComPtr<IDXGISwapChain> _swapChain;

  // 渲染管线
  Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> _samplerState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> _blenderState;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetView;

  // View纹理
  Microsoft::WRL::ComPtr<ID3D11Texture2D> _cefViewTexture;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _cefViewShaderResourceView;
  Microsoft::WRL::ComPtr<ID3D11Buffer> _cefViewVertexBuffer;

  // Popup纹理（用于弹出窗口，如下拉菜单）
  bool _showPopup;
  CefRect _popupRect;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> _cefPopupTexture;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _cefPopupShaderResourceView;
  Microsoft::WRL::ComPtr<ID3D11Buffer> _cefPopupVertexBuffer;
};

} // namespace cefview
```

**Key Resources**:

| 资源类型 | COM接口 | 用途 | 生命周期 |
|---------|---------|------|----------|
| D3D设备 | `ID3D11Device` | 创建资源 | 构造时创建，析构时ComPtr自动Release |
| D3D上下文 | `ID3D11DeviceContext` | 渲染命令 | 同上 |
| 交换链 | `IDXGISwapChain` | 呈现到窗口 | 同上 |
| View纹理 | `ID3D11Texture2D` | 存储CEF渲染内容 | 同上，Resize时重建 |
| 着色器资源视图 | `ID3D11ShaderResourceView` | GPU采样纹理 | 同上 |
| 顶点缓冲区 | `ID3D11Buffer` | 全屏四边形 | 同上 |

**State Diagram**:

```
[Not Initialized]
      │
      ├─ initialize() ──> [Initializing]
      │                       │
      │                       ├─ createDeviceAndSwapchain()
      │                       ├─ createShaderResource()
      │                       ├─ createRenderTargetView()
      │                       └─> [Ready]
      │                              │
      │                              ├─ updateFrameData() ─> [Frame Updated]
      │                              ├─ render() ─────────> [Rendering] ─> [Ready]
      │                              ├─ resize() ─────────> [Resizing] ──> [Ready]
      │                              └─ uninitialize() ───> [Not Initialized]
      │
      └─ ~D3D11Renderer() ──> [Destroyed]
```

---

### 6. MainWindow (Demo Application Window)

**Purpose**: 演示应用的主窗口，管理多个CefWebView实例的布局。

**File Location**: `src/app/MainWindow.h` (已存在，需完善)

**Class Definition**:

```cpp
class MainWindow {
public:
  enum class ShowMode {
    NORMAL,
    MINIMIZED,
    MAXIMIZED,
    FULLSCREEN,
    NO_ACTIVATE
  };

  struct Config {
    ShowMode showMode = ShowMode::NORMAL;
    bool alwaysOnTop = false;
    bool initiallyHidden = false;
    RECT bounds = {};
    RECT sourceBounds = {};
  };

  MainWindow();
  ~MainWindow();

  // 初始化和显示
  void init(std::unique_ptr<Config> config);
  void show(ShowMode mode);
  void hide();
  void setBounds(int x, int y, size_t width, size_t height);
  void close(bool force);

  // DPI管理
  void setDeviceScaleFactor(float deviceScaleFactor);
  float getDeviceScaleFactor() const { return _deviceScaleFactor; }

  HWND getWindowHandle() const { return _hwnd; }

private:
  // 窗口创建
  void createRootWindow(bool initiallyHidden);
  static void RegisterRootClass(HINSTANCE hInstance, const std::wstring& windowClass, HBRUSH backgroundBrush);

  // 窗口过程
  static LRESULT CALLBACK RootWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  // 事件处理
  void onPaint();
  void onSize(bool minimized);
  void onDpiChanged(WPARAM wParam, LPARAM lParam);
  void onClose();
  void onCreate(LPCREATESTRUCT lpCreateStruct);

  // CefWebView管理
  void createCefViews();
  void updateLayout();
  void setActiveBottomView(int index);

private:
  bool _initialized;
  HWND _hwnd;
  float _deviceScaleFactor;
  RECT _initialBounds;
  ShowMode _initialShowMode;

  // CefWebView实例
  std::shared_ptr<CefWebView> _topView;                   // 顶部单个视图
  std::list<std::shared_ptr<CefWebView>> _bottomViews;   // 底部叠加视图列表
  int _activeBottomViewIndex;                             // 当前激活的底部视图索引
};
```

**Layout Model**:

```
MainWindow (1920x1080)
  ├─ Top Area (0, 0, 1920, 540)      ─> _topView (Window Mode)
  │                                      URL: https://www.baidu.com
  │
  └─ Bottom Area (0, 540, 1920, 540) ─> _bottomViews (3个OSR Mode叠加)
       ├─ View 0 (0, 540, 1920, 540) → https://www.google.com  [Z=Top]
       ├─ View 1 (0, 540, 1920, 540) → https://github.com      [Z=Middle]
       └─ View 2 (0, 540, 1920, 540) → https://www.bing.com    [Z=Bottom]
```

**Key Operations**:

```cpp
void MainWindow::createCefViews() {
  // 1. 创建顶部视图（窗口模式）
  CefWebViewSetting topSettings = CefWebViewSetting()
    .setRenderMode(RenderMode::Window)
    .setInitialSize(1920, 540);
  _topView = std::make_shared<CefWebView>("https://www.baidu.com", topSettings, _hwnd);
  _topView->init();

  // 2. 创建底部叠加视图（离屏模式）
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
    _bottomViews.push_back(view);
  }

  _activeBottomViewIndex = 0;
}

void MainWindow::updateLayout() {
  RECT clientRect;
  GetClientRect(_hwnd, &clientRect);
  int width = clientRect.right - clientRect.left;
  int height = clientRect.bottom - clientRect.top;

  int halfHeight = height / 2;

  // 更新顶部视图布局
  _topView->setRect(0, 0, width, halfHeight);

  // 更新底部视图布局（所有视图相同位置，通过Z-order区分）
  for (auto& view : _bottomViews) {
    view->setRect(0, halfHeight, width, halfHeight);
  }
}

void MainWindow::setActiveBottomView(int index) {
  if (index < 0 || index >= _bottomViews.size()) return;

  // 移动选中的视图到列表末尾（绘制在最上层）
  auto it = _bottomViews.begin();
  std::advance(it, index);
  std::shared_ptr<CefWebView> activeView = *it;
  _bottomViews.erase(it);
  _bottomViews.push_back(activeView);

  _activeBottomViewIndex = index;

  // 强制重绘
  InvalidateRect(_hwnd, nullptr, FALSE);
}
```

---

## Relationships & Dependencies

### Class Diagram

```
┌─────────────┐
│ MainWindow  │
└──────┬──────┘
       │ owns (1:N)
       ├──────────────────┐
       │                  │
       ▼                  ▼
┌─────────────┐    ┌─────────────┐
│ CefWebView  │... │ CefWebView  │
│  (top)      │    │  (bottom)   │
└──────┬──────┘    └──────┬──────┘
       │ owns 1:1         │
       │                  │
       ├──────────────────┤
       ▼                  ▼
┌──────────────────────────┐
│ CefViewClientDelegate    │
│  (weak_ptr to CefWebView)│
└────────┬─────────────────┘
         │ injected to
         ▼
┌──────────────────┐
│  CefViewClient   │ ────uses───> CefRefPtr<CefBrowser>
└──────────────────┘
         │ implements
         ▼
┌──────────────────┐
│   CEF Handlers   │
│ (Context, Display│
│  Load, Render... │
└──────────────────┘

CefWebView ────uses (off-screen mode)───> D3D11Renderer
                                              └─> ComPtr<ID3D11Device>
                                              └─> ComPtr<IDXGISwapChain>
                                              └─> ComPtr<ID3D11Texture2D>
```

### Ownership Rules

1. **MainWindow owns CefWebView** (std::shared_ptr)
   - MainWindow负责创建和销毁所有CefWebView实例
   - CefWebView生命周期≤MainWindow生命周期

2. **CefWebView owns CefViewClientDelegate** (std::shared_ptr)
   - Delegate在CefWebView构造时创建
   - Delegate通过weak_ptr引用CefWebView（避免循环）

3. **CefWebView owns D3D11Renderer** (std::unique_ptr)
   - 仅离屏模式创建
   - 独占所有权，不共享

4. **CefViewClient uses CefBrowser** (CefRefPtr)
   - CEF框架管理Browser生命周期
   - Client持有引用防止过早销毁

5. **D3D11Renderer manages COM objects** (ComPtr)
   - 所有D3D11对象通过ComPtr自动管理
   - 析构时自动Release，无需手动Release()

### Threading Model

| 线程 | 职责 | 对象访问 |
|------|------|----------|
| UI Thread | 窗口消息处理、布局更新、D3D11渲染 | MainWindow, CefWebView, D3D11Renderer |
| CEF UI Thread | CEF浏览器事件回调 | CefViewClient, CefViewClientDelegate |
| CEF IO Thread | 网络请求 | CefRequestHandler |
| CEF Render Thread | 网页渲染 | CefRenderProcessHandler |

**Thread Safety**:
- CefWebView的public方法必须在UI线程调用
- CEF回调可能在CEF UI线程，使用CefPostTask转发到UI线程
- D3D11Renderer只在UI线程访问，无需加锁

---

## Data Flow Examples

### Example 1: Load URL in Window Mode

```
User Code:
  webView->loadUrl("https://www.example.com")
    │
    ├─ [Browser not created] ─> Add to _taskListAfterCreated
    │                           └─ Wait for onAfterCreated()
    │
    └─ [Browser ready] ─> _browser->GetMainFrame()->LoadURL(url)
                            │
                            └─> CEF Browser Process
                                  │
                                  ├─ OnLoadStart() ─────────> CefViewClient::OnLoadStart()
                                  │                             │
                                  │                             └─> CefViewClientDelegate::onLoadStart()
                                  │                                   │
                                  │                                   └─> CefWebView::onLoadStart(url)
                                  │
                                  ├─ [网页渲染中...]
                                  │
                                  └─ OnLoadEnd() ───────────> CefViewClient::OnLoadEnd()
                                                                │
                                                                └─> CefViewClientDelegate::onLoadEnd()
                                                                      │
                                                                      └─> CefWebView::onLoadEnd(url)
```

### Example 2: Off-Screen Rendering Flow

```
CEF Render Process:
  [网页内容渲染完成]
    │
    └─> OnPaint(buffer, width, height)  [CEF UI Thread]
          │
          └─> CefViewClient::OnPaint()
                │
                └─> CefViewClientDelegate::onPaint(buffer, width, height)
                      │
                      └─> CefWebView::onPaint(buffer, width, height)
                            │
                            └─> _d3d11Renderer->updateFrameData(buffer, width, height)
                                  │
                                  ├─ _d3dContext->UpdateSubresource(_cefViewTexture, buffer)
                                  │   [GPU Memory] CEF buffer → D3D Texture
                                  │
                                  └─> _d3d11Renderer->render()
                                        │
                                        ├─ _d3dContext->Draw(quad vertices)
                                        │   [GPU] Texture → Render Target
                                        │
                                        └─> _swapChain->Present()
                                              [GPU → Screen]
```

### Example 3: Window Resize

```
Windows Message:
  WM_SIZE
    │
    └─> MainWindow::onSize()
          │
          ├─> updateLayout()
          │     │
          │     ├─> _topView->setRect(0, 0, width, halfHeight)
          │     │     │
          │     │     ├─ [Window Mode] MoveWindow(_cefChildHwnd, ...)
          │     │     │
          │     │     └─ [OSR Mode] _d3d11Renderer->resize(width, halfHeight)
          │     │                      │
          │     │                      ├─> _swapChain->ResizeBuffers(width, halfHeight)
          │     │                      └─> Recreate RenderTargetView
          │     │
          │     └─> for each _bottomViews: view->setRect(0, halfHeight, width, halfHeight)
          │
          └─> InvalidateRect(_hwnd) ─> Trigger WM_PAINT
```

---

## Summary

本数据模型定义了6个核心实体及其关系：

1. **CefWebView** - 主控件类，封装所有浏览器功能
2. **CefWebViewSetting** - 配置对象，Builder模式
3. **CefViewClient** - CEF事件聚合器，实现所有Handler
4. **CefViewClientDelegate** - 事件转发层，解耦CEF和控件
5. **D3D11Renderer** - 离屏渲染后端，硬件加速
6. **MainWindow** - 演示应用窗口，管理多视图布局

**关键设计决策**:
- 使用智能指针严格管理生命周期（shared_ptr, unique_ptr, CefRefPtr, ComPtr）
- Delegate模式解耦CEF底层和应用层
- 任务队列处理异步创建问题
- 清晰的线程模型（UI线程 vs CEF UI线程）

下一阶段将基于此数据模型创建API契约和快速开始指南。
