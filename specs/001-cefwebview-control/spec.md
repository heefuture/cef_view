# Feature Specification: CefWebView Control with Dual Rendering Modes

**Feature Branch**: `001-cefwebview-control`  
**Created**: 2025-11-18  
**Status**: Draft  
**Input**: User description: "我要创建一个基于Windows原生窗口的CefWebView控件, 相关接口在CefWebView.h里面声明。窗口支持离屏渲染和非离屏渲染两种模式，浏览器的交互和拖拽事件需要支持。已有的一些代码需要你完善和修改，保持代码和文件的命名规范和正确性。app的子文件里面需要实现一个测试demo，一个windows原生窗体里面加载多个CefWebView, 顶部和底部分开显示，底部是多个CefWebView叠加显示，只有一个显示在最上层"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 基本浏览器控件创建与URL加载 (Priority: P1)

开发者可以在Windows原生窗口中创建CefWebView控件实例，加载指定URL并显示网页内容。控件可以使用窗口渲染模式（直接渲染到HWND）或离屏渲染模式（渲染到内存缓冲区）。

**Why this priority**: 这是最核心的功能，提供了创建和显示浏览器内容的基础能力。没有这个功能，其他所有功能都无法实现。

**Independent Test**: 创建一个Windows窗口，实例化CefWebView控件并加载URL（如 https://www.baidu.com），验证网页能够正常显示，可以独立完成基本浏览器功能验证。

**Acceptance Scenarios**:

1. **Given** 应用程序已初始化CEF框架，**When** 开发者创建CefWebView实例并指定父窗口句柄、URL和窗口渲染模式，**Then** 控件在指定位置创建子窗口并加载网页内容
2. **Given** 应用程序已初始化CEF框架，**When** 开发者创建CefWebView实例并指定父窗口句柄、URL和离屏渲染模式，**Then** 控件创建离屏渲染缓冲区并开始渲染网页
3. **Given** CefWebView已创建，**When** 调用loadUrl方法加载新URL，**Then** 浏览器导航到新地址并显示内容
4. **Given** CefWebView正在加载网页，**When** 调用getUrl方法，**Then** 返回当前正在加载或已加载的URL
5. **Given** CefWebView已加载网页，**When** 调用refresh方法，**Then** 浏览器重新加载当前页面

---

### User Story 2 - 浏览器控件布局与可见性管理 (Priority: P1)

开发者可以动态调整CefWebView控件的位置、大小和可见性，支持窗口大小变化时的自适应布局。

**Why this priority**: 这是P1优先级因为它是构建实用UI的必要功能，特别是对于演示demo中需要实现的顶部/底部分区布局。

**Independent Test**: 创建一个CefWebView控件，通过setRect方法改变其位置和大小，通过setVisible方法切换显示/隐藏状态，验证控件能够正确响应布局变化。

**Acceptance Scenarios**:

1. **Given** CefWebView已创建并显示，**When** 调用setRect方法设置新的位置和尺寸，**Then** 控件窗口/渲染区域调整到新的矩形区域
2. **Given** CefWebView处于显示状态，**When** 调用setVisible(false)，**Then** 控件变为不可见但保持内部状态
3. **Given** CefWebView处于隐藏状态，**When** 调用setVisible(true)，**Then** 控件恢复显示并继续渲染
4. **Given** 父窗口尺寸改变，**When** 开发者根据新尺寸调用setRect，**Then** 所有CefWebView控件按照新布局正确显示

---

### User Story 3 - 鼠标交互与拖拽事件支持 (Priority: P2)

用户可以在CefWebView控件中进行正常的鼠标交互（点击、滚动、悬停）和拖拽操作（拖拽链接、图片、文本），控件能够正确处理并传递这些事件到CEF浏览器引擎。

**Why this priority**: 交互能力是浏览器控件的核心价值，但基于已有的CEF框架支持，实现优先级低于基本显示功能。

**Independent Test**: 创建CefWebView并加载包含可交互元素的网页（如表单、链接、可拖拽图片），验证用户可以点击链接、填写表单、拖拽元素，且行为与标准浏览器一致。

**Acceptance Scenarios**:

1. **Given** CefWebView加载了包含链接的网页，**When** 用户点击链接，**Then** 浏览器导航到链接目标页面
2. **Given** CefWebView加载了包含输入框的网页，**When** 用户点击输入框并输入文字，**Then** 输入框接收并显示文字内容
3. **Given** CefWebView加载了包含可拖拽元素的网页，**When** 用户拖拽图片或链接，**Then** 显示拖拽光标和拖拽预览，支持拖放操作
4. **Given** CefWebView处于离屏渲染模式，**When** 用户执行鼠标交互，**Then** 控件正确将鼠标事件坐标转换并传递给CEF引擎
5. **Given** CefWebView加载了支持滚动的网页，**When** 用户使用鼠标滚轮滚动，**Then** 页面内容正确滚动

---

### User Story 4 - 多CefWebView叠加显示与Z轴顺序管理 (Priority: P2)

在演示程序中，开发者可以在同一窗口的某个区域（底部）创建多个CefWebView控件并叠加显示，只有一个控件显示在最上层，其他控件被遮挡，可以通过切换改变哪个控件显示在最前面。

**Why this priority**: 这是演示demo的特定需求，展示了控件的实际应用场景和Z轴管理能力。

**Independent Test**: 在演示应用底部区域创建3个叠加的CefWebView（加载不同URL），验证只有顶层控件可见，点击切换按钮后可以将其他控件提升到最前面。

**Acceptance Scenarios**:

1. **Given** 在同一父窗口底部区域创建了3个CefWebView，**When** 设置其中一个控件的Z顺序为最前，**Then** 该控件完全遮挡其他控件且能够接收用户交互
2. **Given** 底部有3个叠加的CefWebView，**When** 用户通过UI操作切换活动控件，**Then** 被选中的控件移动到最前面并接收焦点
3. **Given** 顶部和底部各有CefWebView控件，**When** 调整窗口布局，**Then** 顶部控件独立显示，底部多个控件正确叠加且互不干扰顶部控件

---

### User Story 5 - 浏览器状态监控与控制 (Priority: P3)

开发者可以监控和控制CefWebView的加载状态，包括加载进度、页面标题变化、URL变化等事件，以及执行刷新、停止加载、开发者工具等操作。

**Why this priority**: 这些是增强功能，提供了更完善的浏览器体验，但不影响核心浏览和交互功能。

**Independent Test**: 创建CefWebView并注册状态回调，加载URL后验证能够接收到加载开始、进度、完成、标题变化等事件通知，并能通过stopLoad停止加载。

**Acceptance Scenarios**:

1. **Given** CefWebView注册了加载状态回调，**When** 调用loadUrl加载网页，**Then** 依次触发onLoadStart、onLoadingStateChange、onLoadEnd事件
2. **Given** CefWebView正在加载网页，**When** 页面标题更新，**Then** 触发onTitleChange回调并传递新标题
3. **Given** CefWebView正在加载网页，**When** 调用stopLoad方法，**Then** 停止当前加载并触发相应状态事件
4. **Given** CefWebView已加载网页，**When** 调用openDevTools方法，**Then** 打开Chrome开发者工具窗口用于调试
5. **Given** 开发者工具已打开，**When** 调用closeDevTools方法，**Then** 关闭开发者工具窗口
6. **Given** CefWebView已加载网页，**When** 调用evaluateJavaScript方法执行脚本，**Then** 在网页上下文中执行JavaScript代码

---

### User Story 6 - 演示应用程序实现 (Priority: P3)

提供一个完整的Windows原生窗口演示应用，展示如何使用CefWebView控件：顶部显示一个独立的CefWebView，底部显示多个叠加的CefWebView，包含切换控件的UI按钮。

**Why this priority**: 这是用于测试和演示的应用程序，虽然重要但属于应用层面，不影响控件本身的核心功能。

**Independent Test**: 运行演示应用，验证能够看到顶部和底部的浏览器视图，底部可以通过按钮切换不同的浏览器视图到最前面。

**Acceptance Scenarios**:

1. **Given** 启动演示应用，**When** 应用程序窗口显示，**Then** 顶部显示一个CefWebView（如加载搜索引擎首页），底部显示叠加的多个CefWebView
2. **Given** 演示应用正在运行，**When** 用户调整窗口大小，**Then** 顶部和底部的CefWebView控件按比例自动调整布局
3. **Given** 底部有3个叠加的CefWebView，**When** 用户点击"Tab 1/2/3"切换按钮，**Then** 对应的CefWebView显示在最前面
4. **Given** 用户在顶部CefWebView中浏览网页，**When** 同时点击底部切换按钮，**Then** 两个区域的控件互不干扰，各自正常工作

---

### Edge Cases

- 当CefWebView尚未初始化完成时调用loadUrl等操作方法，如何处理？（需要任务队列延迟执行）
- 当网页加载失败（404、网络错误、证书错误）时，如何通知应用程序？（通过onLoadError回调）
- 当父窗口被销毁时，如何确保CefWebView正确清理资源？（在析构函数和destroy方法中处理）
- 当在离屏渲染模式下，如何处理窗口重绘事件？（需要将渲染缓冲区绘制到目标DC）
- 当同时存在窗口模式和离屏模式的CefWebView时，如何管理渲染性能？（CEF内部优化）
- 当快速切换底部叠加视图的Z顺序时，如何避免闪烁？（使用双缓冲和正确的窗口更新顺序）
- 当拖拽操作跨越多个CefWebView控件边界时，如何正确处理拖拽目标？（需要实现拖拽事件路由）
- 当网页内存占用过大时，如何保护应用程序稳定性？（依赖CEF的内存管理和进程隔离）

## Requirements *(mandatory)*

### Functional Requirements

#### 核心控件功能

- **FR-001**: CefWebView控件MUST支持两种渲染模式：窗口渲染模式（使用子HWND）和离屏渲染模式（OSR，渲染到内存缓冲区）
- **FR-002**: CefWebView控件MUST在创建时接受三个参数：URL字符串、CefWebViewSetting配置对象、父窗口HWND句柄
- **FR-003**: CefWebView控件MUST提供init方法用于完成控件的初始化和CEF浏览器实例创建
- **FR-004**: CefWebView控件MUST提供loadUrl方法用于导航到指定的URL地址
- **FR-005**: CefWebView控件MUST提供getUrl方法用于获取当前页面的URL地址
- **FR-006**: CefWebView控件MUST提供refresh方法用于重新加载当前页面
- **FR-007**: CefWebView控件MUST提供stopLoad方法用于停止当前的页面加载
- **FR-008**: CefWebView控件MUST提供isLoading方法用于查询当前是否正在加载页面

#### 布局与显示管理

- **FR-009**: CefWebView控件MUST提供setRect方法用于设置控件的位置（left, top）和尺寸（width, height）
- **FR-010**: CefWebView控件MUST提供setVisible方法用于控制控件的显示或隐藏状态
- **FR-011**: CefWebView控件MUST在窗口渲染模式下创建Windows子窗口用于显示内容
- **FR-012**: CefWebView控件MUST在离屏渲染模式下将渲染结果绘制到父窗口的指定区域
- **FR-013**: CefWebView控件MUST支持高DPI显示环境，正确处理DPI缩放

#### 交互事件支持

- **FR-014**: CefWebView控件MUST正确处理鼠标点击事件（左键、右键、中键）并传递给CEF引擎
- **FR-015**: CefWebView控件MUST正确处理鼠标移动和悬停事件
- **FR-016**: CefWebView控件MUST正确处理鼠标滚轮滚动事件
- **FR-017**: CefWebView控件MUST支持网页元素的拖拽操作（drag & drop），包括拖拽链接、图片、文本
- **FR-018**: CefWebView控件MUST正确处理键盘输入事件（文字输入、快捷键、导航键）
- **FR-019**: CefWebView控件MUST在离屏渲染模式下正确转换鼠标坐标到浏览器坐标系

#### 状态回调与事件通知

- **FR-020**: CefWebView控件MUST提供onLoadStart回调，在页面开始加载时通知应用程序
- **FR-021**: CefWebView控件MUST提供onLoadEnd回调，在页面加载完成时通知应用程序
- **FR-022**: CefWebView控件MUST提供onLoadError回调，在页面加载失败时通知应用程序错误信息
- **FR-023**: CefWebView控件MUST提供onLoadingStateChange回调，通知加载状态、前进后退按钮可用性变化
- **FR-024**: CefWebView控件MUST提供onTitleChange回调，在页面标题变化时通知应用程序
- **FR-025**: CefWebView控件MUST提供onUrlChange回调，在页面URL变化时通知应用程序（包括hash变化）
- **FR-026**: CefWebView控件MUST提供onAfterCreated回调，在CEF浏览器实例创建完成后通知应用程序
- **FR-027**: CefWebView控件MUST提供onBeforeClose回调，在CEF浏览器实例关闭前通知应用程序

#### 高级功能

- **FR-028**: CefWebView控件MUST提供openDevTools方法用于打开Chrome开发者工具
- **FR-029**: CefWebView控件MUST提供closeDevTools方法用于关闭Chrome开发者工具
- **FR-030**: CefWebView控件MUST提供isDevToolsOpened方法用于查询开发者工具是否已打开
- **FR-031**: CefWebView控件MUST提供evaluateJavaScript方法用于在网页上下文中执行JavaScript代码
- **FR-032**: CefWebView控件MUST提供setZoomLevel方法用于设置页面的缩放级别
- **FR-033**: CefWebView控件MUST提供startDownload方法用于触发文件下载任务
- **FR-034**: CefWebView控件MUST提供getWindowHandle方法用于获取底层窗口句柄（窗口模式）或返回NULL（离屏模式）

#### 资源管理

- **FR-035**: CefWebView控件MUST在析构函数中正确释放CEF浏览器实例和相关资源
- **FR-036**: CefWebView控件MUST在destroy方法中处理提前清理的情况
- **FR-037**: CefWebView控件MUST使用CefRefPtr管理CEF对象的引用计数
- **FR-038**: CefWebView控件MUST在浏览器实例创建前将操作加入任务队列，创建后执行

#### 演示应用程序

- **FR-039**: 演示应用MUST创建一个Windows原生主窗口，包含顶部区域和底部区域
- **FR-040**: 演示应用MUST在顶部区域创建一个独立的CefWebView控件并加载指定URL
- **FR-041**: 演示应用MUST在底部区域创建多个CefWebView控件（建议3个），所有控件叠加在同一位置
- **FR-042**: 演示应用MUST提供UI控制元素（如按钮或标签页）用于切换底部哪个CefWebView显示在最上层
- **FR-043**: 演示应用MUST在窗口大小变化时自动调整顶部和底部CefWebView的布局
- **FR-044**: 演示应用MUST正确处理应用程序的启动、运行和关闭流程，确保CEF正确初始化和清理

#### 代码规范与质量

- **FR-045**: 所有新增和修改的代码MUST遵循项目constitution定义的命名规范（文件名PascalCase，类名PascalCase，方法名camelCase，成员变量下划线前缀）
- **FR-046**: 所有头文件MUST使用#ifndef/#define/#endif头文件保护和#pragma once
- **FR-047**: 所有公开API MUST提供Doxygen格式的注释说明（@brief, @param, @return）
- **FR-048**: 所有代码MUST遵循C++17标准，不使用C++20特性
- **FR-049**: 所有Windows资源（HWND, HRGN等）MUST在析构时正确释放
- **FR-050**: 所有代码MUST使用2空格缩进，每行不超过120字符

### Key Entities

- **CefWebView**: 主要的浏览器控件类，封装CEF浏览器实例，管理窗口、渲染、事件处理。关键属性包括：父窗口句柄、自身窗口句柄（窗口模式）、URL、渲染模式、CEF客户端对象、委托对象、任务队列。

- **CefWebViewSetting**: 控件配置对象，定义CefWebView的创建参数。关键属性包括：渲染模式（窗口/离屏）、是否启用JavaScript、是否启用本地存储、初始尺寸、背景颜色、是否启用GPU加速。

- **CefViewClient**: CEF客户端实现，继承自CefClient，处理CEF的各种回调接口。关键职责包括：生命周期管理、加载事件处理、显示处理、上下文菜单、拖拽处理。

- **CefViewClientDelegate**: 委托对象，将CEF事件转发给CefWebView，实现解耦。关键职责包括：事件转发、状态通知、错误处理。

- **MainWindow** (演示应用): Windows原生主窗口类，管理应用程序主界面。关键属性包括：窗口句柄、顶部CefWebView实例、底部CefWebView实例列表、当前活动视图索引、布局参数。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 开发者能够在5分钟内使用CefWebView控件在Windows窗口中显示一个网页（从创建项目到看到网页内容）

- **SC-002**: CefWebView控件能够在窗口渲染模式下以60 FPS的帧率流畅播放网页动画和视频内容

- **SC-003**: CefWebView控件能够在离屏渲染模式下正确渲染复杂网页（如包含Canvas、WebGL的页面）且性能损失不超过20%

- **SC-004**: 用户在CefWebView控件中执行鼠标点击、拖拽、滚动等操作时，响应延迟小于50毫秒，与原生浏览器体验一致

- **SC-005**: 同一窗口中同时运行5个CefWebView实例（2个窗口模式+3个离屏模式叠加），内存占用不超过800MB，CPU空闲时占用率低于5%

- **SC-006**: 演示应用程序能够在标准1920x1080分辨率和200% DPI缩放环境下正确显示，所有控件清晰无模糊

- **SC-007**: 演示应用中切换底部叠加视图时，切换延迟小于100毫秒，无明显闪烁或卡顿

- **SC-008**: CefWebView控件加载主流网站（百度、谷歌、GitHub）的成功率达到100%，加载时间与Chrome浏览器相当（误差±10%）

- **SC-009**: 所有CefWebView公开API接口都提供完整的文档注释，代码审查时命名规范合规率达到100%

- **SC-010**: 应用程序关闭时，所有CefWebView实例能够在2秒内完成清理，无内存泄漏，无残留子进程

## Assumptions

1. **CEF框架已正确集成**: 假设项目已经包含CEF二进制文件和头文件，libcef_dll_wrapper已编译完成
2. **Windows平台限定**: 假设目标平台为Windows 10/11 64位系统，不考虑跨平台支持
3. **单窗口应用**: 演示应用假设为单主窗口应用，不涉及多窗口、MDI或SDI架构
4. **默认渲染模式**: 当CefWebViewSetting未明确指定渲染模式时，默认使用窗口渲染模式
5. **网络连接可用**: 假设运行环境有可用的网络连接，用于加载在线网页内容
6. **标准URL支持**: 假设加载的URL为标准HTTP/HTTPS协议，不涉及自定义协议或本地文件协议
7. **布局比例**: 演示应用顶部区域占窗口高度的50%，底部区域占50%，左右两侧与窗口边缘对齐
8. **默认加载URL**: 顶部视图加载"https://www.baidu.com"，底部三个视图分别加载"https://www.google.com", "https://github.com", "https://www.bing.com"
9. **Z顺序管理**: 使用Windows API的SetWindowPos/BringWindowToTop管理窗口模式Z顺序，离屏模式通过绘制顺序控制
10. **事件处理模型**: 假设应用程序使用Windows标准消息循环，不涉及多线程UI更新
