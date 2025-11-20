# Research: CefWebView Control with Dual Rendering Modes

**Feature**: 001-cefwebview-control  
**Date**: 2025-11-18  
**Phase**: 0 - Technology Research & Decision Making

## Overview

本文档记录CefWebView控件实现过程中的技术研究、架构决策和最佳实践。重点关注CEF事件传递机制、Direct3D 11离屏渲染实现和Windows原生窗口集成。

## Research Areas

### 1. CEF Framework Architecture & Event Propagation

#### Decision: 基于现有CefClient/CefApp/Delegate架构

**Current Implementation Analysis**:

项目已有完整的CEF事件传递架构：

```
CefViewApp (CefApp)
  ├─ CefBrowserProcessHandler   // 浏览器进程处理
  └─ CefRenderProcessHandler     // 渲染进程处理

CefViewClient (CefClient)
  ├─ CefContextMenuHandler       // 右键菜单
  ├─ CefDisplayHandler           // 显示事件（标题、URL变化）
  ├─ CefDownloadHandler          // 下载管理
  ├─ CefDragHandler              // 拖拽事件
  ├─ CefKeyboardHandler          // 键盘事件
  ├─ CefLifeSpanHandler          // 生命周期（创建、关闭）
  ├─ CefLoadHandler              // 加载事件
  ├─ CefRenderHandler            // 渲染事件（OSR模式）
  └─ CefRequestHandler           // 请求处理

CefViewClientDelegate (CefViewClientDelegateBase)
  └─ 事件转发层，将CEF事件传递给CefWebView控件
```

**Rationale**:
- CEF采用多进程架构（Browser、Renderer、GPU进程），通过IPC通信
- CefClient作为事件聚合器，实现多个Handler接口接收CEF事件
- Delegate模式解耦CEF底层和应用层控件逻辑
- CefRefPtr自动管理CEF对象引用计数，防止内存泄漏

**Implementation Strategy**:
1. **保留现有架构**：CefViewApp、CefViewClient、CefViewClientDelegate
2. **扩展CefWebView**：完善已有的CefWebView类，添加双渲染模式支持
3. **事件回调映射**：
   ```cpp
   CEF Event                      →  CefViewClient Handler  →  CefViewClientDelegate  →  CefWebView Callback
   OnLoadStart()                  →  onLoadStart()          →  onLoadStart()          →  onLoadStart(url)
   OnTitleChange()                →  onTitleChange()        →  onTitleChange()        →  onTitleChange(id, title)
   OnPaint() [OSR]                →  onPaint()              →  updateFrameData()      →  D3D11渲染更新
   ```

**Best Practices**:
- 所有CEF API调用必须在正确的线程（UI线程或IO线程）
- 使用CefPostTask进行线程间任务调度
- CefRefPtr在跨线程传递时会自动增加引用计数
- 浏览器实例创建是异步的，必须在OnAfterCreated回调后才能安全操作

**Alternatives Considered**:
- ❌ 重写完整的CEF封装：工作量大且已有实现稳定
- ❌ 使用第三方CEF封装库：违反constitution仅依赖CEF和Windows API的原则

---

### 2. Direct3D 11 Hardware-Accelerated Off-Screen Rendering

#### Decision: 采用共享纹理方案，参考dx11/DX11RenderBackend实现

**Technology Overview**:

Direct3D 11提供硬件加速的纹理渲染能力，通过共享纹理避免CPU-GPU内存拷贝：

```
CEF GPU Process                  Application Process
     │                                   │
     ├─ Render to Texture              ├─ D3D11 Device/Context
     │  (GPU memory)                    │
     ├─ OnPaint() callback              ├─ Create Shared Texture
     │  └─ BGRA buffer                  │  └─ D3D11_TEXTURE2D
     │                                   │
     └─ Copy buffer to texture ─────────┤
        (UpdateSubresource)              │
                                         ├─ Render Texture to Window
                                         │  └─ Swap Chain Present
                                         │
                                         └─ Handle WM_PAINT
```

**Reference Implementation** (`dx11/DX11RenderBackend.cpp`):

关键技术点：
1. **D3D11 Device Creation**:
   ```cpp
   D3D11CreateDevice(
     nullptr,                      // 默认适配器
     D3D_DRIVER_TYPE_HARDWARE,     // 硬件加速
     nullptr,                      // 软件光栅化器句柄（不使用）
     0,                            // 创建标志
     nullptr, 0,                   // Feature Levels（自动选择）
     D3D11_SDK_VERSION,
     &pD3dDevice, nullptr, &pD3dContext
   );
   ```

2. **Swap Chain for Window**:
   ```cpp
   DXGI_SWAP_CHAIN_DESC1 sd = {};
   sd.BufferCount = 2;                        // 双缓冲
   sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;    // BGRA格式（匹配CEF）
   sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   sd.SampleDesc.Count = 1;                   // 无MSAA
   sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Flip模型（高性能）
   
   pDxgiFactory->CreateSwapChainForHwnd(
     pD3dDevice, hWnd, &sd, nullptr, nullptr, &m_swapChain
   );
   ```

3. **Texture Creation for CEF Frame**:
   ```cpp
   D3D11_TEXTURE2D_DESC texDesc = {};
   texDesc.Width = width;
   texDesc.Height = height;
   texDesc.MipLevels = 1;
   texDesc.ArraySize = 1;
   texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;  // BGRA（CEF格式）
   texDesc.SampleDesc.Count = 1;
   texDesc.Usage = D3D11_USAGE_DEFAULT;
   texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // 用作纹理采样
   texDesc.CPUAccessFlags = 0;
   
   pD3dDevice->CreateTexture2D(&texDesc, nullptr, &m_cefViewTexture);
   ```

4. **Frame Data Update** (from CEF OnPaint):
   ```cpp
   void UpdateFrameData(const void* buffer, int width, int height) {
     D3D11_BOX destRegion = {0, 0, 0, (UINT)width, (UINT)height, 1};
     m_d3dContext->UpdateSubresource(
       m_cefViewTexture.Get(),     // 目标纹理
       0,                           // Mip level
       &destRegion,                 // 更新区域
       buffer,                      // CEF BGRA buffer
       width * 4,                   // Row pitch (4 bytes per pixel)
       0                            // Depth pitch
     );
   }
   ```

5. **Render Pipeline**:
   - Vertex Shader: 简单的全屏四边形
   - Pixel Shader: 纹理采样
   - Input Layout: Position + TexCoord
   - Render Target: Swap Chain Back Buffer

**Rationale**:
- **性能**: GPU硬件加速，避免CPU软件渲染
- **零拷贝**: 纹理在GPU内存中，Present时无需CPU参与
- **成熟技术**: D3D11是Windows平台标准图形API，广泛支持
- **兼容性**: 支持Windows 7 SP1+（Feature Level 11.0）

**Implementation Plan for CefWebView**:
1. 创建`D3D11Renderer`类（放置在`src/cefview/osr_renderer/`）
2. 在CefWebView构造时，根据CefWebViewSetting选择渲染模式：
   - 窗口模式：CEF直接渲染到子HWND
   - 离屏模式：创建D3D11Renderer，接收OnPaint数据
3. 实现CefRenderHandler::OnPaint回调：
   ```cpp
   void CefViewClient::OnPaint(
     CefRefPtr<CefBrowser> browser,
     PaintElementType type,
     const RectList& dirtyRects,
     const void* buffer,
     int width, int height
   ) {
     if (_d3d11Renderer) {
       _d3d11Renderer->updateFrameData(type, dirtyRects, buffer, width, height);
       _d3d11Renderer->render();  // 触发Present
     }
   }
   ```
4. 窗口WM_PAINT处理：
   ```cpp
   case WM_PAINT: {
     if (_renderMode == RenderMode::OffScreen) {
       ValidateRect(_hwnd, nullptr);  // 标记已绘制
       // D3D11Renderer已在OnPaint中Present，无需额外操作
     }
     return 0;
   }
   ```

**Best Practices**:
- 使用`Microsoft::WRL::ComPtr`管理D3D11 COM对象，自动Release
- 检查HRESULT返回值，处理设备丢失（DeviceRemoved）
- 在窗口Resize时，重建Swap Chain和Render Target View
- 使用DXGI_SWAP_EFFECT_FLIP_* 以获得最佳性能
- 纹理格式必须匹配CEF的BGRA（B8G8R8A8_UNORM）

**Alternatives Considered**:
- ❌ **GDI/GDI+ Software Rendering**: 性能差，CPU负担重，无法满足60 FPS目标
- ❌ **OpenGL**: 需要额外依赖（第三方上下文管理），Windows平台D3D11是首选
- ❌ **Direct2D**: 更适合2D矢量图形，对于纹理绘制D3D11更直接
- ✅ **Direct3D 11**: Windows原生API，硬件加速，性能最优，零额外依赖

---

### 3. Windows Native Window Management & Event Routing

#### Decision: 使用标准Win32窗口层次结构和消息路由

**Window Hierarchy**:

```
MainWindow (HWND)                    // 应用主窗口
  ├─ Top Area                        // 顶部区域（50%高度）
  │   └─ CefWebView #1 (Window Mode) 
  │       └─ Child HWND              // CEF创建的子窗口
  │
  └─ Bottom Area                     // 底部区域（50%高度）
      ├─ CefWebView #2 (OSR Mode)    // 叠加视图1（Z-order高）
      ├─ CefWebView #3 (OSR Mode)    // 叠加视图2
      └─ CefWebView #4 (OSR Mode)    // 叠加视图3（Z-order低）
```

**Window Mode Implementation**:

窗口模式下，CEF自动创建子窗口：
```cpp
void CefWebView::createCefBrowser(const std::string& url, const CefWebViewSetting& settings) {
  CefWindowInfo windowInfo;
  if (settings.renderMode == RenderMode::Window) {
    // 窗口渲染模式：CEF在指定区域创建子HWND
    windowInfo.SetAsChild(_hwnd, {left, top, left + width, top + height});
  } else {
    // 离屏渲染模式：无窗口
    windowInfo.SetAsWindowless(nullptr);
  }
  
  CefBrowserHost::CreateBrowser(windowInfo, _client, url, ...);
}
```

**Off-Screen Mode Implementation**:

离屏模式需要手动处理鼠标/键盘事件：
```cpp
LRESULT CefWebView::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (_renderMode == RenderMode::OffScreen && _browser) {
    CefRefPtr<CefBrowserHost> host = _browser->GetHost();
    
    switch (msg) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MOUSEMOVE: {
        CefMouseEvent mouseEvent;
        mouseEvent.x = GET_X_LPARAM(lp);
        mouseEvent.y = GET_Y_LPARAM(lp);
        mouseEvent.modifiers = getCefMouseModifiers(wp);
        
        if (msg == WM_LBUTTONDOWN)
          host->SendMouseClickEvent(mouseEvent, MBT_LEFT, false, 1);
        else if (msg == WM_RBUTTONDOWN)
          host->SendMouseClickEvent(mouseEvent, MBT_RIGHT, false, 1);
        else
          host->SendMouseMoveEvent(mouseEvent, false);
        break;
      }
      
      case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        host->SendMouseWheelEvent(mouseEvent, 0, delta);
        break;
      }
      
      case WM_KEYDOWN:
      case WM_CHAR: {
        CefKeyEvent keyEvent;
        // 填充keyEvent字段
        host->SendKeyEvent(keyEvent);
        break;
      }
    }
  }
  
  return DefWindowProc(hwnd, msg, wp, lp);
}
```

**Z-Order Management** (for stacked OSR views):

```cpp
void MainWindow::setActiveBottomView(int index) {
  // 方案1: 改变绘制顺序
  std::swap(_bottomViews[index], _bottomViews.back());
  // 最后一个绘制在最上层
  
  // 方案2: 使用SetWindowPos（如果每个OSR view有独立HWND）
  for (size_t i = 0; i < _bottomViews.size(); ++i) {
    SetWindowPos(
      _bottomViews[i]->getWindowHandle(),
      (i == index) ? HWND_TOP : HWND_BOTTOM,
      0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
  }
}
```

**DPI Awareness**:

```cpp
// 在WinMain初始化时
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

// 在WM_DPICHANGED处理
case WM_DPICHANGED: {
  UINT newDpi = HIWORD(wParam);
  float scaleFactor = newDpi / 96.0f;
  
  // 调整所有CefWebView的DPI缩放
  for (auto& view : _allViews) {
    view->setDeviceScaleFactor(scaleFactor);
  }
  
  // 调整窗口大小
  RECT* newRect = reinterpret_cast<RECT*>(lParam);
  SetWindowPos(hwnd, nullptr, 
    newRect->left, newRect->top,
    newRect->right - newRect->left,
    newRect->bottom - newRect->top,
    SWP_NOZORDER | SWP_NOACTIVATE);
  break;
}
```

**Best Practices**:
- 使用`SetUserDataPtr`将CefWebView指针存储在HWND的GWLP_USERDATA
- 窗口类注册时设置WNDCLASSEX.hbrBackground = (HBRUSH)(COLOR_WINDOW+1)
- 在WM_SIZE中调用所有CefWebView的setRect更新布局
- 离屏模式鼠标事件坐标需要相对于CefWebView的左上角
- 使用UpdateWindow强制立即重绘，避免闪烁

**Alternatives Considered**:
- ❌ **MFC/ATL窗口类**: 引入额外依赖，违反constitution
- ❌ **WTL**: 轻量但仍是第三方库
- ✅ **Pure Win32**: 零依赖，完全控制，符合constitution要求

---

### 4. Resource Lifecycle & RAII Pattern

#### Decision: 严格使用智能指针和RAII模式管理所有资源

**Resource Types & Management**:

| 资源类型 | 管理方式 | 示例 |
|---------|---------|------|
| CEF对象 | `CefRefPtr<T>` | `CefRefPtr<CefBrowser>` |
| C++对象 | `std::shared_ptr<T>`, `std::unique_ptr<T>` | `std::shared_ptr<CefWebView>` |
| D3D11 COM | `Microsoft::WRL::ComPtr<T>` | `ComPtr<ID3D11Device>` |
| HWND | RAII包装类或析构函数DestroyWindow | `~CefWebView() { if (_hwnd) DestroyWindow(_hwnd); }` |
| HRGN | 析构函数DeleteObject | `~MainWindow() { if (_region) DeleteObject(_region); }` |

**CefWebView Lifecycle**:

```cpp
class CefWebView : public std::enable_shared_from_this<CefWebView> {
public:
  CefWebView(...) {
    // 1. 存储参数但不创建浏览器
    _url = url;
    _settings = settings;
    _parentHwnd = parentHwnd;
  }
  
  void init() {
    // 2. 创建CEF浏览器（异步）
    createCefBrowser(_url, _settings);
    // 浏览器创建完成后，CefViewClientDelegate::onAfterCreated被调用
  }
  
  void onAfterCreated(int browserId) {
    // 3. 浏览器就绪，执行pending任务队列
    for (auto& task : _taskListAfterCreated) {
      task();
    }
    _taskListAfterCreated.clear();
  }
  
  ~CefWebView() {
    // 4. 清理资源
    destroy();
  }
  
private:
  void destroy() {
    if (_browser) {
      _browser->GetHost()->CloseBrowser(true);  // 强制关闭
      _browser = nullptr;
    }
    if (_hwnd && _renderMode == RenderMode::OffScreen) {
      DestroyWindow(_hwnd);  // 离屏模式的容器窗口
      _hwnd = nullptr;
    }
    _d3d11Renderer.reset();  // 释放D3D11资源
  }
  
  CefRefPtr<CefBrowser> _browser;                  // CEF管理
  std::unique_ptr<D3D11Renderer> _d3d11Renderer;   // 独占所有权
  HWND _hwnd;                                       // 手动管理
};
```

**D3D11Renderer Lifecycle**:

```cpp
class D3D11Renderer {
public:
  D3D11Renderer(HWND hwnd, int width, int height) {
    if (!createDeviceAndSwapchain(hwnd, width, height)) {
      throw std::runtime_error("Failed to create D3D11 device");
    }
  }
  
  ~D3D11Renderer() {
    // ComPtr自动Release所有COM对象
    // 无需手动调用Release()
  }
  
private:
  ComPtr<ID3D11Device> _d3dDevice;          // 自动Release
  ComPtr<ID3D11DeviceContext> _d3dContext;  // 自动Release
  ComPtr<IDXGISwapChain> _swapChain;        // 自动Release
  // ... 其他ComPtr成员
};
```

**Task Queue Pattern** (for async CEF initialization):

```cpp
void CefWebView::loadUrl(const std::string& url) {
  if (!_browser) {
    // 浏览器尚未创建，加入任务队列
    _taskListAfterCreated.push_back([this, url]() {
      this->loadUrl(url);  // 创建完成后再调用
    });
    return;
  }
  
  // 浏览器已创建，立即执行
  _browser->GetMainFrame()->LoadURL(url);
}
```

**Best Practices**:
- 永远不要手动调用`delete`，使用智能指针
- CEF对象必须用CefRefPtr，不能用std::shared_ptr（引用计数机制不同）
- D3D11对象必须用ComPtr，不要手动Release（会导致double-free）
- HWND在析构时检查有效性再DestroyWindow
- 使用RAII包装类管理Windows资源（如RAII_HRGN）

---

## Technology Stack Summary

| 组件 | 技术选择 | 版本/要求 |
|------|---------|----------|
| 编程语言 | C++17 | MSVC 2022, /std:c++17 |
| 浏览器引擎 | CEF (Chromium Embedded Framework) | 项目已集成版本 |
| 图形API | Direct3D 11 | Feature Level 11.0+ |
| 窗口系统 | Win32 API | Windows 10/11 SDK |
| 构建系统 | CMake | 3.14+ |
| 智能指针 | std::shared_ptr, std::unique_ptr, CefRefPtr, ComPtr | C++17 标准库 + CEF + WRL |

## Implementation Priorities

**Phase 1: 窗口模式核心功能** (P1)
- CefWebView窗口模式创建和URL加载
- 基本API实现（loadUrl, refresh, setRect等）
- CefViewClientDelegate事件回调映射

**Phase 2: 离屏渲染支持** (P1)
- D3D11Renderer类实现（参考dx11/DX11RenderBackend）
- CefRenderHandler::OnPaint集成
- 鼠标/键盘事件路由

**Phase 3: 多视图管理** (P2)
- MainWindow布局管理（顶部/底部分区）
- Z-order切换逻辑
- DPI感知和缩放

**Phase 4: 高级功能** (P3)
- 开发者工具（openDevTools/closeDevTools）
- JavaScript执行（evaluateJavaScript）
- 下载管理（startDownload）

## Risk Mitigation

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| D3D11设备丢失 | 中 | 高 | 实现HandleDeviceLost，重建设备和资源 |
| CEF异步初始化导致调用失败 | 高 | 中 | 使用任务队列延迟执行，onAfterCreated后处理 |
| 离屏模式事件坐标转换错误 | 中 | 中 | 严格测试坐标映射，考虑DPI缩放 |
| 多实例内存占用超标 | 低 | 中 | 监控内存使用，CEF内部已优化共享资源 |
| Windows消息循环阻塞CEF | 低 | 高 | 使用CefDoMessageLoopWork或CefRunMessageLoop |

## Conclusion

本研究确认了基于现有CEF架构和Direct3D 11的技术方案是可行且最优的。关键决策点：
- ✅ 复用现有CefViewClient/CefViewApp/Delegate架构，降低实现风险
- ✅ 采用D3D11硬件加速离屏渲染，满足性能要求（50+ FPS）
- ✅ 使用纯Win32 API，符合constitution零第三方UI框架依赖
- ✅ 严格RAII和智能指针管理，确保资源安全

下一阶段将基于本研究创建详细的数据模型和API契约文档。
