# CefWebView API Contract

**Feature**: 001-cefwebview-control  
**Date**: 2025-11-18  
**Phase**: 1 - API Design & Contract

## Overview

本文档定义CefWebView控件的公开API接口契约，包括方法签名、参数验证、返回值、异常处理和线程安全性要求。

## API Categories

### 1. Construction & Initialization

#### 1.1 Constructor

```cpp
namespace cefview {

/**
 * @brief 构造CefWebView控件实例
 * @param url 初始加载的URL地址
 * @param settings 控件配置参数
 * @param parentHwnd 父窗口句柄
 * @throws std::invalid_argument 如果parentHwnd无效或settings包含非法值
 */
CefWebView::CefWebView(
  const std::string& url,
  const CefWebViewSetting& settings,
  HWND parentHwnd
);

}
```

**Preconditions**:
- `parentHwnd` MUST be a valid window handle (`IsWindow(parentHwnd) == TRUE`)
- `url` SHOULD be a valid HTTP/HTTPS URL (允许为空，后续通过loadUrl加载)
- `settings.initialWidth` and `settings.initialHeight` MUST be > 0

**Postconditions**:
- CefWebView实例已创建但浏览器尚未初始化
- 内部状态为`[Constructed]`
- `_client`和`_clientDelegate`已创建

**Thread Safety**: 必须在UI线程调用

---

#### 1.2 init()

```cpp
/**
 * @brief 初始化CEF浏览器实例（异步操作）
 * @note 必须在构造后立即调用，浏览器创建完成后会触发onAfterCreated回调
 */
void CefWebView::init();
```

**Preconditions**:
- CefWebView实例已构造
- `init()`尚未被调用（不可重复初始化）

**Postconditions**:
- 异步创建CEF浏览器实例
- 状态变为`[Initializing]`
- 创建完成后状态变为`[Ready]`，并调用`onAfterCreated(browserId)`

**Behavior**:
- 根据`_settings.renderMode`选择创建窗口模式或离屏模式浏览器
- 窗口模式：CEF创建子HWND
- 离屏模式：创建D3D11Renderer，设置windowless模式

**Thread Safety**: 必须在UI线程调用

**Example**:
```cpp
auto webView = std::make_shared<CefWebView>("https://www.baidu.com", settings, parentHwnd);
webView->init();  // 异步初始化，稍后onAfterCreated被调用
```

---

### 2. Navigation & Content Loading

#### 2.1 loadUrl()

```cpp
/**
 * @brief 导航到指定URL
 * @param url 要加载的URL地址（必须是有效的HTTP/HTTPS协议）
 * @note 如果浏览器尚未创建，操作会被加入任务队列，创建完成后自动执行
 */
void CefWebView::loadUrl(const std::string& url);
```

**Preconditions**:
- `url` MUST NOT be empty
- `url` SHOULD be a valid HTTP/HTTPS URL (data: 和 file: 协议取决于CEF配置)

**Postconditions**:
- 如果`_browser`已创建：立即调用`_browser->GetMainFrame()->LoadURL(url)`
- 如果`_browser`未创建：加入`_taskListAfterCreated`，等待`onAfterCreated`后执行
- 触发`onLoadStart(url)`事件

**Thread Safety**: 必须在UI线程调用

**Error Handling**:
- 如果URL格式非法，CEF会触发`onLoadError`回调

---

#### 2.2 getUrl()

```cpp
/**
 * @brief 获取当前页面的URL
 * @return 当前URL字符串（可能与loadUrl传入的URL不同，如重定向或hash变化）
 */
const std::string& CefWebView::getUrl() const;
```

**Preconditions**: None

**Postconditions**:
- 返回`_url`成员变量（由`onUrlChange`回调更新）

**Thread Safety**: 必须在UI线程调用（const引用，但成员变量可能被修改）

---

#### 2.3 refresh()

```cpp
/**
 * @brief 刷新当前页面
 * @note 等同于用户按F5，会重新加载页面资源
 */
void CefWebView::refresh();
```

**Preconditions**: `_browser` SHOULD be created (否则操作无效)

**Postconditions**:
- 调用`_browser->Reload()`
- 触发`onLoadStart`和`onLoadEnd`事件序列

**Thread Safety**: 必须在UI线程调用

---

#### 2.4 stopLoad()

```cpp
/**
 * @brief 停止当前页面加载
 */
void CefWebView::stopLoad();
```

**Preconditions**: `_browser` SHOULD be created

**Postconditions**:
- 调用`_browser->StopLoad()`
- 如果正在加载，触发`onLoadEnd`（httpStatusCode可能为0）

**Thread Safety**: 必须在UI线程调用

---

#### 2.5 isLoading()

```cpp
/**
 * @brief 查询当前是否正在加载页面
 * @return true表示正在加载，false表示加载完成或空闲
 */
bool CefWebView::isLoading() const;
```

**Preconditions**: None

**Postconditions**:
- 返回`_browser->IsLoading()`（如果browser未创建返回false）

**Thread Safety**: 必须在UI线程调用

---

### 3. Layout & Visibility Management

#### 3.1 setRect()

```cpp
/**
 * @brief 设置控件的位置和尺寸
 * @param left 相对于父窗口的左边位置（像素）
 * @param top 相对于父窗口的上边位置（像素）
 * @param width 控件宽度（像素，必须>0）
 * @param height 控件高度（像素，必须>0）
 * @throws std::invalid_argument 如果width或height <= 0
 */
void CefWebView::setRect(int left, int top, int width, int height);
```

**Preconditions**:
- `width` > 0 AND `height` > 0

**Postconditions**:
- **窗口模式**：调用`MoveWindow(_cefChildHwnd, left, top, width, height, TRUE)`
- **离屏模式**：调用`_d3d11Renderer->resize(width, height)`，重建交换链和纹理
- 通知CEF浏览器：`_browser->GetHost()->WasResized()`

**Thread Safety**: 必须在UI线程调用

**Example**:
```cpp
webView->setRect(0, 0, 1024, 768);  // 设置为1024x768，位于父窗口(0,0)位置
```

---

#### 3.2 setVisible()

```cpp
/**
 * @brief 控制控件的显示或隐藏
 * @param visible true显示，false隐藏
 */
void CefWebView::setVisible(bool visible);
```

**Preconditions**: None

**Postconditions**:
- **窗口模式**：调用`ShowWindow(_cefChildHwnd, visible ? SW_SHOW : SW_HIDE)`
- **离屏模式**：调用`ShowWindow(_hwnd, visible ? SW_SHOW : SW_HIDE)`（容器窗口）
- 通知CEF：`_browser->GetHost()->WasHidden(!visible)`

**Thread Safety**: 必须在UI线程调用

---

### 4. Advanced Features

#### 4.1 openDevTools()

```cpp
/**
 * @brief 打开Chrome开发者工具
 * @return true成功，false失败（如浏览器未创建）
 */
bool CefWebView::openDevTools();
```

**Preconditions**: `_browser` SHOULD be created

**Postconditions**:
- 打开新窗口显示DevTools
- `_isDevToolsOpened`设为true
- 返回true表示成功

**Thread Safety**: 必须在UI线程调用

---

#### 4.2 closeDevTools()

```cpp
/**
 * @brief 关闭Chrome开发者工具
 */
void CefWebView::closeDevTools();
```

**Preconditions**: DevTools已打开

**Postconditions**:
- 关闭DevTools窗口
- `_isDevToolsOpened`设为false

**Thread Safety**: 必须在UI线程调用

---

#### 4.3 evaluateJavaScript()

```cpp
/**
 * @brief 在网页上下文中执行JavaScript代码
 * @param script JavaScript代码字符串
 * @note 执行是异步的，无返回值。如需返回值，使用CEF的ProcessMessage机制
 */
void CefWebView::evaluateJavaScript(const std::string& script);
```

**Preconditions**:
- `script` MUST NOT be empty
- `_browser` SHOULD be created

**Postconditions**:
- 调用`_browser->GetMainFrame()->ExecuteJavaScript(script, "", 0)`
- 脚本在网页主框架的JavaScript上下文中执行

**Thread Safety**: 必须在UI线程调用

**Example**:
```cpp
webView->evaluateJavaScript("document.title = 'Hello from C++';");
```

---

#### 4.4 setZoomLevel()

```cpp
/**
 * @brief 设置页面缩放级别
 * @param zoomLevel 缩放级别（0.0为100%，正值放大，负值缩小）
 * @note CEF zoomLevel定义：0.0=100%, 1.0≈110%, -1.0≈90%
 */
void CefWebView::setZoomLevel(float zoomLevel);
```

**Preconditions**: `_browser` SHOULD be created

**Postconditions**:
- 调用`_browser->GetHost()->SetZoomLevel(zoomLevel)`
- 页面内容按指定级别缩放

**Thread Safety**: 必须在UI线程调用

---

#### 4.5 startDownload()

```cpp
/**
 * @brief 触发文件下载任务
 * @param url 要下载的文件URL
 */
void CefWebView::startDownload(const std::string& url);
```

**Preconditions**:
- `url` MUST be a valid HTTP/HTTPS URL
- `_browser` SHOULD be created

**Postconditions**:
- 调用`_browser->GetHost()->StartDownload(url)`
- 触发`CefDownloadHandler`回调（`onBeforeDownload`, `onDownloadUpdated`）

**Thread Safety**: 必须在UI线程调用

---

#### 4.6 getWindowHandle()

```cpp
/**
 * @brief 获取底层窗口句柄
 * @return 窗口模式返回CEF创建的子HWND，离屏模式返回容器HWND
 */
CefWindowHandle CefWebView::getWindowHandle() const;
```

**Preconditions**: None

**Postconditions**:
- **窗口模式**：返回`_browser->GetHost()->GetWindowHandle()`（CEF子窗口）
- **离屏模式**：返回`_hwnd`（容器窗口，用于接收鼠标/键盘事件）

**Thread Safety**: 必须在UI线程调用

---

### 5. Event Callbacks (Invoked by CefViewClientDelegate)

以下方法由`CefViewClientDelegate`调用，开发者通常不直接调用这些方法，但可以继承CefWebView并重写以自定义行为。

#### 5.1 onAfterCreated()

```cpp
/**
 * @brief CEF浏览器实例创建完成回调
 * @param browserId CEF分配的浏览器ID
 * @note 此时浏览器已就绪，可以安全调用所有API
 */
void CefWebView::onAfterCreated(int browserId);
```

**Invoked When**: CEF浏览器创建完成（异步）

**Actions**:
- 执行`_taskListAfterCreated`中的所有pending任务
- 清空任务队列
- 状态从`[Initializing]`变为`[Ready]`

**Thread Context**: CEF UI Thread（由CefLifeSpanHandler::OnAfterCreated触发）

---

#### 5.2 onLoadStart()

```cpp
/**
 * @brief 页面开始加载回调
 * @param url 正在加载的URL
 */
void CefWebView::onLoadStart(const std::string& url);
```

**Invoked When**: 浏览器开始加载新页面

**Thread Context**: CEF UI Thread

---

#### 5.3 onLoadEnd()

```cpp
/**
 * @brief 页面加载完成回调
 * @param url 已加载的URL
 */
void CefWebView::onLoadEnd(const std::string& url);
```

**Invoked When**: 页面加载完成（包括所有子资源）

**Thread Context**: CEF UI Thread

---

#### 5.4 onLoadError()

```cpp
/**
 * @brief 页面加载失败回调
 * @param browserId 浏览器ID
 * @param errorText 错误描述文本
 * @param failedUrl 失败的URL
 */
void CefWebView::onLoadError(int browserId, const std::string& errorText, const std::string& failedUrl);
```

**Invoked When**: 页面加载失败（404, 网络错误, 证书错误等）

**Thread Context**: CEF UI Thread

---

#### 5.5 onTitleChange()

```cpp
/**
 * @brief 页面标题变化回调
 * @param browserId 浏览器ID
 * @param title 新标题
 */
void CefWebView::onTitleChange(int browserId, const std::string& title);
```

**Invoked When**: 网页<title>标签内容变化

**Thread Context**: CEF UI Thread

---

#### 5.6 onUrlChange()

```cpp
/**
 * @brief URL变化回调
 * @param browserId 浏览器ID
 * @param oldUrl 旧URL
 * @param url 新URL
 */
void CefWebView::onUrlChange(int browserId, const std::string& oldUrl, const std::string& url);
```

**Invoked When**: URL变化（包括hash变化、重定向、前进/后退）

**Actions**:
- 更新`_url`成员变量

**Thread Context**: CEF UI Thread

---

## API Usage Patterns

### Pattern 1: Basic Browser Creation

```cpp
// 1. 定义配置
CefWebViewSetting settings = CefWebViewSetting()
  .setRenderMode(RenderMode::Window)
  .setInitialSize(800, 600)
  .setBackgroundColor(0xFFFFFFFF);

// 2. 创建控件（在MainWindow的onCreate中）
auto webView = std::make_shared<CefWebView>("https://www.baidu.com", settings, _hwnd);

// 3. 初始化（异步）
webView->init();

// 4. 后续操作（可以立即调用，会自动延迟到浏览器就绪）
webView->loadUrl("https://www.google.com");  // 加入任务队列
webView->setRect(0, 0, 1024, 768);           // 加入任务队列

// 5. onAfterCreated被调用后，任务队列自动执行
```

### Pattern 2: Off-Screen Rendering with D3D11

```cpp
CefWebViewSetting settings = CefWebViewSetting()
  .setRenderMode(RenderMode::OffScreen)
  .setInitialSize(1920, 1080)
  .setBackgroundColor(0xFF000000);  // 黑色背景

auto webView = std::make_shared<CefWebView>("https://webglsamples.org/aquarium/aquarium.html", settings, _hwnd);
webView->init();

// D3D11Renderer会在init()中自动创建
// CEF的OnPaint回调会更新D3D纹理并Present到窗口
```

### Pattern 3: Multi-View Management

```cpp
// MainWindow中管理多个叠加的离屏视图
std::list<std::shared_ptr<CefWebView>> _bottomViews;

void createBottomViews() {
  std::vector<std::string> urls = {
    "https://www.google.com",
    "https://github.com",
    "https://www.bing.com"
  };

  CefWebViewSetting settings = CefWebViewSetting()
    .setRenderMode(RenderMode::OffScreen)
    .setInitialSize(1920, 540);

  for (const auto& url : urls) {
    auto view = std::make_shared<CefWebView>(url, settings, _hwnd);
    view->init();
    _bottomViews.push_back(view);
  }
}

// 切换激活视图（Z-order管理）
void setActiveBottomView(int index) {
  auto it = _bottomViews.begin();
  std::advance(it, index);
  std::shared_ptr<CefWebView> activeView = *it;
  
  // 移动到列表末尾（最后绘制，显示在最上层）
  _bottomViews.erase(it);
  _bottomViews.push_back(activeView);
  
  // 强制重绘
  InvalidateRect(_hwnd, nullptr, FALSE);
}

// WM_PAINT处理（离屏模式手动绘制顺序决定Z-order）
void onPaint() {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(_hwnd, &ps);
  
  // 按列表顺序绘制（后绘制的遮挡先绘制的）
  for (auto& view : _bottomViews) {
    // D3D11Renderer已在OnPaint回调中Present，这里无需操作
    // 或者可以显式调用view->render()触发绘制
  }
  
  EndPaint(_hwnd, &ps);
}
```

### Pattern 4: JavaScript Interaction

```cpp
// 执行JavaScript
webView->evaluateJavaScript(R"(
  console.log('Hello from C++');
  document.body.style.backgroundColor = 'lightblue';
)");

// 如果需要从JS返回值给C++，使用ProcessMessage机制
// （需要在renderer进程中注册handler）
```

---

## Error Handling & Edge Cases

### Error Conditions

| 错误情况 | API行为 | 推荐处理 |
|---------|---------|----------|
| 浏览器未创建时调用导航API | 加入任务队列，延迟执行 | 正常流程，无需特殊处理 |
| loadUrl传入非法URL | CEF触发onLoadError回调 | 重写onLoadError处理错误 |
| setRect传入width/height<=0 | 抛出std::invalid_argument | 调用前验证参数 |
| D3D11设备丢失 | D3D11Renderer::handleDeviceLost重建设备 | 自动恢复，无需应用层处理 |
| 父窗口被销毁 | 在CefWebView析构时调用destroy()清理 | 确保MainWindow析构时清理所有CefWebView |

### Thread Safety Violations

**禁止行为**:
- ❌ 在非UI线程调用CefWebView的任何public方法
- ❌ 手动调用`onXxx`系列回调方法（仅由Delegate调用）
- ❌ 在CefWebView析构后访问其成员（使用weak_ptr检查有效性）

**安全模式**:
- ✅ 所有API调用都在UI线程（MainWindow的WndProc或消息处理函数中）
- ✅ CEF回调自动路由到CEF UI线程，Delegate负责线程同步
- ✅ 使用`std::weak_ptr`持有CefWebView引用，避免悬空指针

---

## API Versioning & Stability

**Current Version**: 1.0.0

**Stability Guarantees**:
- 所有public方法签名保持向后兼容（Semantic Versioning）
- 新增方法不破坏现有代码
- Deprecated方法保留至少一个Major版本

**Breaking Changes** (future):
- 参数类型变化：Major版本递增
- 方法删除：Major版本递增
- 行为语义变化：Minor版本递增+文档说明

---

## Summary

本API契约定义了CefWebView的26个公开方法和8个事件回调，涵盖：
- ✅ 构造与初始化（2个）
- ✅ 导航与内容加载（5个）
- ✅ 布局与可见性（2个）
- ✅ 高级功能（6个）
- ✅ 事件回调（8个）

**关键设计原则**:
- 异步初始化：浏览器创建是异步的，任务队列机制保证API调用顺序
- 线程安全：所有API必须在UI线程调用，CEF回调由框架处理线程同步
- RAII资源管理：智能指针自动管理生命周期，无需手动清理
- 明确前置/后置条件：API契约清晰定义预期行为

下一步将创建快速开始指南，展示如何使用这些API构建演示应用。
