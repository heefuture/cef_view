# Tasks: CefWebView Control with Dual Rendering Modes

**Feature**: 001-cefwebview-control  
**Input**: Design documents from `/specs/001-cefwebview-control/`  
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/cefwebview-api.md

**Tests**: 本功能采用手动测试（通过演示应用验证），不包含自动化测试任务。

**Organization**: 任务按用户故事分组，使每个故事能够独立实现和测试。

## Format: `[ID] [P?] [Story] Description`

- **[P]**: 可并行运行（不同文件，无依赖）
- **[Story]**: 任务所属用户故事（US1-US6）
- 包含精确文件路径

## Path Conventions

项目采用单项目结构：
- 核心库：`src/cefview/`
- 演示应用：`src/app/`
- 参考实现：`dx11/` (仅参考，不修改)

---

## Phase 1: Setup (项目初始化)

**Purpose**: 完成项目基础设施和构建配置

- [X] T001 验证CEF SDK是否正确集成到libcef/目录
- [X] T002 验证CMakeLists.txt是否包含D3D11库链接配置（d3d11.lib, dxgi.lib, d3dcompiler.lib）
- [X] T003 [P] 创建src/cefview/osr_renderer目录（如不存在）
- [X] T004 [P] 验证现有文件命名规范符合constitution（PascalCase）

---

## Phase 2: Foundational (核心基础设施) ⚠️ BLOCKING

**Purpose**: 必须完成的基础组件，阻塞所有用户故事

**⚠️ CRITICAL**: 在此阶段完成前，任何用户故事都不能开始实现

### 基础渲染器实现

- [X] T005 [P] 创建D3D11Renderer.h头文件在src/cefview/osr_renderer/D3D11Renderer.h
- [X] T006 创建D3D11Renderer.cpp实现文件在src/cefview/osr_renderer/D3D11Renderer.cpp
- [X] T007 [D3D11Renderer.cpp] 实现createDeviceAndSwapchain()方法（参考dx11/DX11RenderBackend.cpp）
- [X] T008 [P] [D3D11Renderer.cpp] 实现createShaderResource()方法创建纹理和着色器资源视图
- [X] T009 [P] [D3D11Renderer.cpp] 实现createRenderTargetView()方法
- [X] T010 [P] [D3D11Renderer.cpp] 实现createSampler()和createBlender()方法
- [X] T011 [D3D11Renderer.cpp] 实现setupPipeline()方法配置渲染管线
- [X] T012 [D3D11Renderer.cpp] 实现updateFrameData()方法接收CEF的OnPaint缓冲区
- [X] T013 [D3D11Renderer.cpp] 实现render()方法将纹理绘制到窗口
- [X] T014 [D3D11Renderer.cpp] 实现resize()方法处理窗口大小变化
- [X] T015 [D3D11Renderer.cpp] 实现handleDeviceLost()方法处理设备丢失恢复
- [X] T016 [P] [D3D11Renderer.cpp] 实现updatePopupVisibility()和updatePopupRect()支持弹出窗口
- [X] T017 [D3D11Renderer.cpp] 实现析构函数正确释放所有COM对象（ComPtr自动管理）

### 委托层完善

- [X] T018 [P] 完善CefViewClientDelegate.h在src/cefview/view/CefViewClientDelegate.h添加onPaint()纯虚方法
- [X] T019 [CefViewClientDelegate.cpp] 实现onPaint()方法转发给CefWebView
- [X] T020 [P] [CefViewClientDelegate.cpp] 实现onGetViewRect()方法返回视图矩形
- [X] T021 [P] [CefViewClient.cpp] 在GetRenderHandler()中返回this（离屏模式需要）
- [X] T022 [CefViewClient.cpp] 实现OnPaint()回调调用delegate->onPaint()
- [X] T023 [CefViewClient.cpp] 实现GetViewRect()回调调用delegate->onGetViewRect()

**Checkpoint**: 基础设施就绪 - 用户故事实现现在可以并行开始

---

## Phase 3: User Story 1 - 基本浏览器控件创建与URL加载 (Priority: P1) 🎯 MVP

**Goal**: 实现核心CefWebView控件，支持窗口渲染和离屏渲染两种模式，能够创建控件实例并加载URL显示网页。

**Independent Test**: 创建一个最小的Windows应用，实例化CefWebView（窗口模式），加载https://www.baidu.com，验证网页能正常显示并可交互。

### Implementation for User Story 1

#### 核心控件类实现

- [X] T024 [P] [US1] 完善CefWebView.h在src/cefview/view/CefWebView.h添加RenderMode枚举（Window/OffScreen)
- [X] T025 [P] [US1] [CefWebView.h] 添加私有成员变量：_renderMode, _d3d11Renderer, _taskListAfterCreated
- [X] T026 [US1] [CefWebView.cpp] 实现构造函数CefWebView(url, settings, parentHwnd)
- [X] T027 [US1] [CefWebView.cpp] 在构造函数中创建CefViewClient和CefViewClientDelegate
- [X] T028 [US1] [CefWebView.cpp] 实现init()方法异步创建CEF浏览器
- [X] T029 [US1] [CefWebView.cpp] 在init()中根据renderMode选择创建窗口或离屏浏览器
- [X] T030 [US1] [CefWebView.cpp] 窗口模式：调用createSubWindow()创建子窗口
- [X] T031 [US1] [CefWebView.cpp] 离屏模式：创建D3D11Renderer实例并初始化
- [X] T032 [US1] [CefWebView.cpp] 实现createCefBrowser()方法配置CefBrowserSettings

#### 导航控制方法

- [X] T033 [P] [US1] [CefWebView.cpp] 实现loadUrl()方法
- [X] T034 [P] [US1] [CefWebView.cpp] 在loadUrl()中检查_browser是否已创建，未创建则加入_taskListAfterCreated
- [X] T035 [P] [US1] [CefWebView.cpp] 实现getUrl()方法返回_url成员
- [X] T036 [P] [US1] [CefWebView.cpp] 实现refresh()方法调用_browser->Reload()
- [X] T037 [P] [US1] [CefWebView.cpp] 实现stopLoad()方法调用_browser->StopLoad()
- [X] T038 [P] [US1] [CefWebView.cpp] 实现isLoading()方法查询_browser->IsLoading()

#### 事件回调实现

- [X] T039 [P] [US1] [CefWebView.cpp] 实现onAfterCreated()回调
- [X] T040 [US1] [CefWebView.cpp] 在onAfterCreated()中执行_taskListAfterCreated中所有pending任务
- [X] T041 [P] [US1] [CefWebView.cpp] 实现onLoadStart()回调更新状态
- [X] T042 [P] [US1] [CefWebView.cpp] 实现onLoadEnd()回调更新状态
- [X] T043 [P] [US1] [CefWebView.cpp] 实现onLoadError()回调记录错误信息
- [X] T044 [P] [US1] [CefWebView.cpp] 实现onUrlChange()回调更新_url成员变量

#### 离屏渲染集成

- [X] T045 [US1] [CefWebView.cpp] 实现onPaint()方法调用_d3d11Renderer->updateFrameData()
- [X] T046 [US1] [CefWebView.cpp] 在WM_PAINT消息中调用_d3d11Renderer->render()

#### 资源管理

- [X] T047 [US1] [CefWebView.cpp] 实现destroy()方法关闭CEF浏览器
- [X] T048 [US1] [CefWebView.cpp] 实现析构函数调用destroy()并释放D3D11Renderer
- [X] T049 [US1] [CefWebView.cpp] 实现onBeforeClose()回调清理资源

#### 代码质量

- [ ] T050 [P] [US1] 为CefWebView所有公开方法添加Doxygen注释（@brief, @param, @return）
- [ ] T051 [P] [US1] 验证CefWebView.h使用#ifndef/#define/#endif和#pragma once双重保护
- [ ] T052 [P] [US1] 验证所有文件使用2空格缩进，每行不超过120字符

**Checkpoint**: 此时US1应完全功能可用 - 可以创建CefWebView控件（窗口/离屏模式）并加载URL

---

## Phase 4: User Story 2 - 浏览器控件布局与可见性管理 (Priority: P1)

**Goal**: 实现CefWebView的布局控制能力，支持动态调整位置、大小和可见性，适应窗口大小变化。

**Independent Test**: 创建CefWebView后，通过setRect()改变其位置和大小，通过setVisible()切换显示/隐藏，验证控件能正确响应。

### Implementation for User Story 2

- [X] T053 [P] [US2] [CefWebView.cpp] 实现setRect()方法
- [X] T054 [US2] [CefWebView.cpp] 在setRect()中窗口模式调用MoveWindow()移动子窗口
- [X] T055 [US2] [CefWebView.cpp] 在setRect()中离屏模式调用_d3d11Renderer->resize()
- [X] T056 [US2] [CefWebView.cpp] 在setRect()后通知CEF调用_browser->GetHost()->WasResized()
- [X] T057 [P] [US2] [CefWebView.cpp] 实现setVisible()方法
- [X] T058 [US2] [CefWebView.cpp] 在setVisible()中窗口模式调用ShowWindow()控制子窗口可见性
- [X] T059 [US2] [CefWebView.cpp] 在setVisible()中离屏模式调用ShowWindow()控制容器窗口
- [X] T060 [US2] [CefWebView.cpp] 在setVisible()后通知CEF调用_browser->GetHost()->WasHidden()
- [X] T061 [P] [US2] [CefWebView.cpp] 实现getWindowHandle()方法返回窗口句柄
- [X] T062 [P] [US2] 添加输入验证：setRect()的width和height必须>0，否则抛出std::invalid_argument

**Checkpoint**: 此时US1和US2均应独立可用 - 控件支持完整的布局管理

---

## Phase 5: User Story 3 - 鼠标交互与拖拽事件支持 (Priority: P2)

**Goal**: 实现鼠标交互事件处理，包括点击、移动、滚动和拖拽操作，确保CEF浏览器能正确接收和处理这些事件。

**Independent Test**: 创建CefWebView加载包含可交互元素的网页（表单、链接、可拖拽图片），验证用户可以点击链接、填写表单、拖拽元素。

### Implementation for User Story 3

#### 鼠标事件处理

- [X] T063 [P] [US3] [CefWebView.cpp] 添加窗口过程处理WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN
- [X] T064 [P] [US3] [CefWebView.cpp] 添加窗口过程处理WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP
- [X] T065 [P] [US3] [CefWebView.cpp] 添加窗口过程处理WM_MOUSEMOVE
- [X] T066 [P] [US3] [CefWebView.cpp] 添加窗口过程处理WM_MOUSEWHEEL滚动事件
- [X] T067 [US3] [CefWebView.cpp] 在离屏模式下将鼠标坐标转换到浏览器坐标系
- [X] T068 [US3] [CefWebView.cpp] 调用_browser->GetHost()->SendMouseClickEvent()发送点击事件
- [X] T069 [US3] [CefWebView.cpp] 调用_browser->GetHost()->SendMouseMoveEvent()发送移动事件
- [X] T070 [US3] [CefWebView.cpp] 调用_browser->GetHost()->SendMouseWheelEvent()发送滚轮事件

#### 拖拽事件支持

- [X] T071 [P] [US3] [CefViewClient.cpp] 实现CefDragHandler接口方法OnDragEnter()
- [X] T072 [P] [US3] [CefViewClient.cpp] 实现OnDraggableRegionsChanged()方法
- [X] T073 [US3] [CefWebView.cpp] 在窗口过程中处理WM_DROPFILES消息
- [X] T074 [US3] [CefWebView.cpp] 调用_browser->GetHost()->DragTargetDragEnter()转发拖拽事件

#### 键盘事件支持

- [X] T075 [P] [US3] [CefWebView.cpp] 添加窗口过程处理WM_KEYDOWN, WM_KEYUP
- [X] T076 [P] [US3] [CefWebView.cpp] 添加窗口过程处理WM_CHAR字符输入
- [X] T077 [US3] [CefWebView.cpp] 调用_browser->GetHost()->SendKeyEvent()发送键盘事件

**Checkpoint**: 此时US1-US3均应独立可用 - 控件支持完整的用户交互

---

## Phase 6: User Story 4 - 多CefWebView叠加显示与Z轴顺序管理 (Priority: P2)

**Goal**: 在演示应用底部区域实现多个CefWebView叠加显示，支持Z轴顺序切换,只有顶层视图可见并接收交互。

**Independent Test**: 在演示应用底部创建3个叠加的CefWebView（加载不同URL），验证只有顶层可见，点击切换按钮后可以将其他控件提升到最前面。

**Note**: 此Phase为演示应用UI专属功能，基础MainWindow.cpp已存在，可根据需求实现。

### Implementation for User Story 4

- [ ] T078 [US4] [MainWindow.cpp] 添加_bottomViews成员变量（std::list<std::shared_ptr<CefWebView>>）
- [ ] T079 [US4] [MainWindow.cpp] 添加_activeBottomViewIndex成员变量（int）
- [ ] T080 [US4] [MainWindow.cpp] 实现createBottomViews()方法创建3个离屏模式CefWebView
- [ ] T081 [US4] [MainWindow.cpp] 在createBottomViews()中为每个视图设置相同的矩形区域（叠加）
- [ ] T082 [US4] [MainWindow.cpp] 实现setActiveBottomView(int index)方法
- [ ] T083 [US4] [MainWindow.cpp] 在setActiveBottomView()中将选中视图移动到列表末尾（绘制在最上层）
- [ ] T084 [US4] [MainWindow.cpp] 在setActiveBottomView()后调用InvalidateRect()触发重绘
- [ ] T085 [P] [US4] [MainWindow.cpp] 在WM_PAINT处理中按_bottomViews列表顺序绘制（离屏模式D3D11自动Present）
- [ ] T086 [P] [US4] [MainWindow.cpp] 添加按钮或键盘快捷键触发setActiveBottomView()切换

**Checkpoint**: CefWebView控件核心功能已完备，演示应用UI可根据需求独立实现

---

## Phase 7: User Story 5 - 浏览器状态监控与控制 (Priority: P3)

**Goal**: 实现浏览器状态监控和控制功能，包括加载进度、标题变化、开发者工具、JavaScript执行等高级特性。

**Independent Test**: 创建CefWebView并注册状态回调，加载URL后验证能接收到加载开始、进度、完成、标题变化等事件，并能通过stopLoad停止加载。

### Implementation for User Story 5

#### 状态回调实现

- [X] T087 [P] [US5] [CefWebView.cpp] 实现onTitleChange()回调更新窗口标题（如果需要）
- [X] T088 [P] [US5] [CefWebView.cpp] 实现onLoadingStateChange()回调更新加载状态标志
- [X] T089 [P] [US5] [CefViewClient.cpp] 实现OnLoadingStateChange()转发给delegate

#### 开发者工具

- [X] T090 [P] [US5] [CefWebView.cpp] 实现openDevTools()方法
- [X] T091 [US5] [CefWebView.cpp] 在openDevTools()中调用_browser->GetHost()->ShowDevTools()
- [X] T092 [US5] [CefWebView.cpp] 设置_isDevToolsOpened标志为true
- [X] T093 [P] [US5] [CefWebView.cpp] 实现closeDevTools()方法调用_browser->GetHost()->CloseDevTools()
- [X] T094 [P] [US5] [CefWebView.cpp] 实现isDevToolsOpened()方法返回_isDevToolsOpened

#### JavaScript交互

- [X] T095 [P] [US5] [CefWebView.cpp] 实现evaluateJavaScript()方法
- [X] T096 [US5] [CefWebView.cpp] 在evaluateJavaScript()中调用_browser->GetMainFrame()->ExecuteJavaScript()
- [X] T097 [P] [US5] [CefWebView.cpp] 添加输入验证：script不能为空

#### 其他高级功能

- [X] T098 [P] [US5] [CefWebView.cpp] 实现setZoomLevel()方法调用_browser->GetHost()->SetZoomLevel()
- [X] T099 [P] [US5] [CefWebView.cpp] 实现startDownload()方法调用_browser->GetHost()->StartDownload()
- [X] T100 [P] [US5] [CefViewClient.cpp] 实现CefDownloadHandler接口方法OnBeforeDownload()
- [X] T101 [P] [US5] [CefViewClient.cpp] 实现OnDownloadUpdated()方法

**Checkpoint**: 此时US1-US5均应独立可用 - 控件支持完整的状态监控和高级控制

---

## Phase 8: User Story 6 - 演示应用程序实现 (Priority: P3)

**Goal**: 实现完整的Windows原生窗口演示应用，展示CefWebView控件的所有功能：顶部独立视图+底部叠加视图+切换UI。

**Independent Test**: 运行演示应用，验证能看到顶部和底部的浏览器视图，底部可以通过按钮切换不同的浏览器视图到最前面，窗口大小变化时布局自动调整。

### Implementation for User Story 6

#### 主窗口基础

- [ ] T102 [US6] [MainWindow.cpp] 完善createRootWindow()方法创建主窗口
- [ ] T103 [US6] [MainWindow.cpp] 在RootWndProc中处理WM_CREATE消息调用onCreate()
- [ ] T104 [US6] [MainWindow.cpp] 实现onCreate()方法初始化CEF和创建CefWebView实例
- [ ] T105 [US6] [MainWindow.cpp] 在onCreate()中调用createCefViews()创建顶部和底部视图

#### 顶部视图创建

- [ ] T106 [P] [US6] [MainWindow.cpp] 实现createTopView()方法创建窗口模式CefWebView
- [ ] T107 [US6] [MainWindow.cpp] 在createTopView()中加载默认URL（https://www.baidu.com）
- [ ] T108 [US6] [MainWindow.cpp] 设置顶部视图初始矩形为窗口上半部分

#### 布局管理

- [ ] T109 [US6] [MainWindow.cpp] 实现updateLayout()方法
- [ ] T110 [US6] [MainWindow.cpp] 在updateLayout()中计算顶部/底部区域矩形（各占50%高度）
- [ ] T111 [US6] [MainWindow.cpp] 调用_topView->setRect()更新顶部视图布局
- [ ] T112 [US6] [MainWindow.cpp] 遍历_bottomViews调用setRect()更新底部视图布局
- [ ] T113 [US6] [MainWindow.cpp] 在WM_SIZE消息处理中调用updateLayout()

#### 切换UI实现

- [ ] T114 [P] [US6] [MainWindow.cpp] 添加切换按钮或标签页UI控件（使用Windows原生Button/Tab控件）
- [ ] T115 [US6] [MainWindow.cpp] 处理按钮点击消息（WM_COMMAND）调用setActiveBottomView()
- [ ] T116 [US6] [MainWindow.cpp] 或实现键盘快捷键（Ctrl+1/2/3）切换视图

#### DPI支持

- [ ] T117 [P] [US6] [MainWindow.cpp] 在WM_DPICHANGED消息中调用onDpiChanged()
- [ ] T118 [US6] [MainWindow.cpp] 实现onDpiChanged()方法更新_deviceScaleFactor
- [ ] T119 [US6] [MainWindow.cpp] 通知所有CefWebView更新DPI设置

#### 应用生命周期

- [ ] T120 [US6] [main.cpp] 实现WinMain入口点初始化CEF
- [ ] T121 [US6] [main.cpp] 调用CefInitialize()配置CEF
- [ ] T122 [US6] [main.cpp] 创建MainWindow实例并显示
- [ ] T123 [US6] [main.cpp] 运行Windows消息循环
- [ ] T124 [US6] [main.cpp] 在退出时调用CefShutdown()清理CEF资源
- [ ] T125 [US6] [MainWindow.cpp] 在WM_CLOSE消息中确保所有CefWebView正确销毁
- [ ] T126 [US6] [MainWindow.cpp] 实现close()方法调用DestroyWindow()

**Checkpoint**: 演示应用完整功能可用 - 所有用户故事集成并验证

---

## Phase 9: Polish & Cross-Cutting Concerns

**Purpose**: 代码质量、性能优化和文档完善

### 代码质量

- [ ] T127 [P] 验证所有头文件包含顺序符合constitution（相关头→C系统→C++标准→CEF→D3D→项目头）
- [ ] T128 [P] 运行代码格式化工具验证2空格缩进和120字符行宽
- [ ] T129 [P] 验证所有类名、方法名、成员变量符合命名规范（PascalCase, camelCase, _前缀）
- [ ] T130 [P] 检查所有CefRefPtr和ComPtr使用正确，无裸指针和手动Release
- [ ] T131 [P] 验证所有= delete禁用拷贝构造符合RAII原则

### 性能验证

- [ ] T132 验证窗口模式渲染帧率达到60 FPS（加载动画网页测试）
- [ ] T133 验证离屏模式渲染帧率达到50+ FPS（加载WebGL网页测试）
- [ ] T134 验证5个CefWebView实例总内存占用≤800MB（使用任务管理器监控）
- [ ] T135 验证鼠标点击响应延迟<50ms（使用高速摄像或测试工具）
- [ ] T136 验证演示应用关闭时2秒内完成清理，无残留进程（检查CEF子进程）

### 文档完善

- [ ] T137 [P] 验证quickstart.md中的示例代码能够编译运行
- [ ] T138 [P] 更新README.md添加功能特性说明和快速开始链接
- [ ] T139 [P] 为CefWebView.h, D3D11Renderer.h, MainWindow.h添加文件头注释（@file, @brief, @author, @date）

### 边缘情况处理

- [ ] T140 测试浏览器未创建时调用loadUrl()能正确加入任务队列
- [ ] T141 测试加载404页面触发onLoadError回调
- [ ] T142 测试父窗口销毁时CefWebView正确清理资源
- [ ] T143 测试快速切换底部视图无闪烁
- [ ] T144 测试D3D11设备丢失恢复（模拟：切换显示器、改变分辨率）
- [ ] T145 测试高DPI环境（200%缩放）显示清晰无模糊

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: 无依赖 - 可立即开始
- **Foundational (Phase 2)**: 依赖Setup完成 - **阻塞所有用户故事**
- **User Stories (Phase 3-8)**: 全部依赖Foundational完成
  - US1-US6可以按优先级顺序实现
  - 或在团队资源充足时并行实现（US1/US2可完全并行）
- **Polish (Phase 9)**: 依赖所有期望用户故事完成

### User Story Dependencies

- **US1 (P1)**: Foundational完成后可开始 - 无其他故事依赖 ✅ MVP核心
- **US2 (P1)**: Foundational完成后可开始 - 无其他故事依赖 ✅ MVP核心
- **US3 (P2)**: 依赖US1完成（需要基本控件） - 可独立测试
- **US4 (P2)**: 依赖US1完成（需要基本控件） - 可独立测试
- **US5 (P3)**: 依赖US1完成（需要基本控件） - 可独立测试
- **US6 (P3)**: 依赖US1, US2, US4完成（需要完整控件和多视图管理） - 集成测试

### Within Each User Story

- **US1**: T024-T026 (头文件定义) → T027-T032 (构造和初始化) → T033-T038 (导航方法) → T039-T044 (事件回调) → T045-T046 (离屏渲染) → T047-T049 (资源管理) → T050-T052 (代码质量)
- **US2**: T053-T056 (setRect实现) 可与 T057-T060 (setVisible实现) 并行
- **US3**: 鼠标事件处理 (T063-T070) 可与拖拽支持 (T071-T074) 和键盘支持 (T075-T077) 并行
- **US4**: T078-T081 (数据结构和创建) → T082-T084 (切换逻辑) → T085-T086 (UI集成)
- **US5**: 状态回调 (T087-T089) 可与开发者工具 (T090-T094)、JavaScript (T095-T097)、其他功能 (T098-T101) 并行
- **US6**: 主窗口 (T102-T105) → 顶部视图 (T106-T108) 可与布局 (T109-T113) 并行 → 切换UI (T114-T116) 可与DPI (T117-T119) 并行 → 生命周期 (T120-T126)

### Parallel Opportunities

#### Foundational Phase (T005-T023)

```bash
# D3D11Renderer方法可并行实现（不同方法）：
T008 [P] createShaderResource()
T009 [P] createRenderTargetView()
T010 [P] createSampler() + createBlender()
T016 [P] updatePopupVisibility() + updatePopupRect()

# Delegate层可与D3D11并行：
T018 [P] CefViewClientDelegate.h
T020 [P] onGetViewRect()
T021 [P] GetRenderHandler()
```

#### User Story 1 (T024-T052)

```bash
# 头文件定义和成员变量可并行：
T024 [P] RenderMode枚举
T025 [P] 私有成员变量

# 导航方法可并行实现：
T033 [P] loadUrl()
T035 [P] getUrl()
T036 [P] refresh()
T037 [P] stopLoad()
T038 [P] isLoading()

# 事件回调可并行实现：
T041 [P] onLoadStart()
T042 [P] onLoadEnd()
T043 [P] onLoadError()
T044 [P] onUrlChange()

# 代码质量任务可并行：
T050 [P] Doxygen注释
T051 [P] 头文件保护
T052 [P] 格式验证
```

#### User Story 3 (T063-T077)

```bash
# 鼠标事件处理可并行：
T063 [P] 鼠标按下事件
T064 [P] 鼠标抬起事件
T065 [P] 鼠标移动
T066 [P] 鼠标滚轮

# 拖拽支持可并行：
T071 [P] OnDragEnter()
T072 [P] OnDraggableRegionsChanged()

# 键盘支持可并行：
T075 [P] WM_KEYDOWN/UP
T076 [P] WM_CHAR
```

#### User Story 5 (T087-T101)

```bash
# 状态回调可并行：
T087 [P] onTitleChange()
T088 [P] onLoadingStateChange()

# 高级功能可并行：
T090 [P] openDevTools()
T093 [P] closeDevTools()
T094 [P] isDevToolsOpened()
T095 [P] evaluateJavaScript()
T098 [P] setZoomLevel()
T099 [P] startDownload()
T100 [P] OnBeforeDownload()
T101 [P] OnDownloadUpdated()
```

---

## Implementation Strategy

### MVP First (US1 + US2 Only)

最小可用产品只需实现US1和US2：

1. ✅ Complete Phase 1: Setup (T001-T004)
2. ✅ Complete Phase 2: Foundational (T005-T023) - **关键阻塞点**
3. ✅ Complete Phase 3: User Story 1 (T024-T052) - 基本浏览器控件
4. ✅ Complete Phase 4: User Story 2 (T053-T062) - 布局管理
5. **STOP and VALIDATE**: 
   - 创建简单的Windows应用
   - 实例化CefWebView加载网页
   - 验证窗口模式和离屏模式都能正常显示
   - 验证setRect()和setVisible()工作正常
6. **MVP READY**: 可以发布基础版本或演示

### Incremental Delivery

按优先级逐步添加功能：

1. **Foundation** (Phase 1-2) → 基础设施就绪
2. **MVP** (US1+US2) → 基本浏览器控件 → 测试 → 演示 ✅
3. **Enhanced Interaction** (US3) → 添加交互支持 → 测试 → 演示
4. **Multi-View** (US4) → 添加多视图管理 → 测试 → 演示
5. **Advanced Features** (US5) → 添加高级功能 → 测试 → 演示
6. **Demo App** (US6) → 完整演示应用 → 测试 → 发布 🎉

每个阶段独立可测试，不破坏之前的功能。

### Parallel Team Strategy

如果有多个开发者：

1. **全团队**: 完成Setup + Foundational (Phase 1-2) 一起工作
2. **Foundational完成后分工**:
   - Developer A: US1 (核心控件) → 阻塞其他故事
   - Developer B: 协助US1或准备US2文档
3. **US1完成后**:
   - Developer A: US2 (布局管理)
   - Developer B: US3 (交互事件) - 可与US2并行
   - Developer C: US4 (多视图管理) - 可与US2/US3并行
4. **US1-US4完成后**:
   - Developer A: US5 (高级功能)
   - Developer B: US6 (演示应用)
   - Developer C: Polish (Phase 9)

---

## Task Summary

**总任务数**: 145个任务

**任务分布**:
- Phase 1 (Setup): 4个任务
- Phase 2 (Foundational): 19个任务 ⚠️ 阻塞关键
- Phase 3 (US1 - P1): 29个任务 🎯 MVP核心
- Phase 4 (US2 - P1): 10个任务 🎯 MVP核心
- Phase 5 (US3 - P2): 15个任务
- Phase 6 (US4 - P2): 9个任务
- Phase 7 (US5 - P3): 15个任务
- Phase 8 (US6 - P3): 25个任务
- Phase 9 (Polish): 19个任务

**并行机会**:
- Foundational阶段: 约12个可并行任务
- US1阶段: 约18个可并行任务
- US2阶段: 约6个可并行任务
- US3阶段: 约12个可并行任务
- US5阶段: 约11个可并行任务
- **总计**: 约60个任务可并行执行（占41%）

**MVP范围** (推荐首次交付):
- Phase 1-2 (Setup + Foundational): 23个任务
- Phase 3-4 (US1 + US2): 39个任务
- **MVP总计**: 62个任务（占43%）

**预计工作量**:
- 单开发者顺序实现: 约3-4周（假设每天完成5-7个任务）
- MVP交付: 约1.5-2周
- 完整功能: 约3-4周

---

## Notes

- [P] 标记的任务可以并行执行（不同文件，无依赖）
- [Story] 标签映射任务到具体用户故事，便于追溯
- 每个用户故事应独立可完成和测试
- 在每个Checkpoint验证故事独立功能
- 优先完成MVP（US1+US2），然后增量添加功能
- 严格遵循constitution的命名和代码规范
- 所有资源使用智能指针管理（CefRefPtr, ComPtr, shared_ptr, unique_ptr）
- 参考dx11/目录实现但不修改这些文件
- 提交频率：每完成一个逻辑单元（如一个方法或一组相关方法）
- 避免：模糊任务、同文件冲突、破坏独立性的跨故事依赖
