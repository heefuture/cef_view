# Implementation Plan: CefWebView Control with Dual Rendering Modes

**Branch**: `001-cefwebview-control` | **Date**: 2025-11-18 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/001-cefwebview-control/spec.md`

## Summary

本功能实现一个基于Windows原生窗口的CefWebView浏览器控件，支持窗口渲染和离屏渲染两种模式。核心采用CEF框架的事件传递和回调机制，通过CefClient、CefApp和相关delegate实现完整的浏览器功能。离屏渲染模式使用Direct3D 11硬件加速共享纹理技术，实现高性能网页内容渲染。演示应用展示了单窗口中多个CefWebView控件的管理和布局。

**技术方案**:
- **核心框架**: 基于现有CefViewClient、CefViewApp和CefViewClientDelegate的CEF事件传递架构
- **离屏渲染**: 参考项目根目录`dx11/`文件夹的DX11RenderBackend实现，采用D3D11共享纹理方案
- **窗口管理**: 使用Windows原生Win32 API创建和管理窗口层次结构
- **资源管理**: 严格遵循RAII模式，使用智能指针和CefRefPtr管理生命周期

## Technical Context

**Language/Version**: C++17 (严格遵循标准，不使用C++20特性)

**Primary Dependencies**: 
- CEF (Chromium Embedded Framework) - 浏览器引擎
- Direct3D 11 - 离屏渲染硬件加速
- Windows SDK (Win32/User32) - 原生窗口管理
- d3dcompiler.lib - HLSL着色器编译
- dxgi.lib, d3d11.lib - Direct3D接口

**Storage**: 不涉及持久化存储（CEF内部管理缓存和Cookie）

**Testing**: 手动测试（通过演示应用验证所有功能场景）

**Target Platform**: Windows 10/11 64位系统，支持Direct3D 11 Feature Level 11.0或更高

**Project Type**: 单项目结构（Windows桌面应用程序）

**Performance Goals**: 
- 窗口模式：60 FPS流畅渲染
- 离屏模式：50+ FPS（D3D11硬件加速）
- 多实例：5个CefWebView实例内存占用≤800MB
- 事件响应延迟：<50ms

**Constraints**: 
- 必须使用C++17标准（不使用C++20）
- 仅依赖CEF、D3D11和Windows原生API
- 不引入第三方UI框架
- 所有代码遵循项目constitution定义的编码规范
- 必须正确管理Windows资源和COM对象生命周期

**Scale/Scope**: 
- 核心控件库：约2000-3000行代码
- 演示应用：约500-800行代码
- 支持单窗口内同时运行5+个CefWebView实例
- 主要文件数量：10-15个（头文件+实现文件）

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### ✅ Principle I: C++17 Standard Compliance
- 所有代码严格使用C++17标准特性
- 使用std::optional, std::variant, structured bindings等现代C++特性
- 不使用C++20的concepts、ranges、coroutines等特性
- **Status**: PASS - 技术栈已明确限定为C++17

### ✅ Principle II: CEF Framework Integration
- 使用现有的CefViewClient继承CefClient及各Handler接口
- 通过CefViewApp实现CefApp、CefBrowserProcessHandler、CefRenderProcessHandler
- 使用CefViewClientDelegate作为委托层，解耦CEF事件和控件逻辑
- 正确管理CefRefPtr引用计数
- **Status**: PASS - 基于现有CEF架构扩展

### ✅ Principle III: Windows Native UI
- 使用CreateWindowEx创建窗口
- 通过WndProc处理WM_*消息
- 使用HWND作为窗口句柄类型
- 支持高DPI通过GetDpiForWindow等原生API
- **Status**: PASS - 纯Windows原生API实现

### ✅ Principle IV: Object-Oriented Design
- CefWebView类封装浏览器控件逻辑（单一职责）
- CefViewClientDelegate作为委托模式实现（解耦CEF和控件）
- CefWebViewSetting作为配置对象（数据封装）
- 使用组合而非继承（CefWebView包含CefViewClient）
- **Status**: PASS - 清晰的类层次和职责划分

### ✅ Principle V: Naming Conventions Consistency
- 文件名：PascalCase (CefWebView.h, CefWebView.cpp, DX11RenderBackend.h)
- 类名：PascalCase (CefWebView, CefViewClient, MainWindow)
- 方法名：camelCase (loadUrl, setRect, createSubWindow)
- 成员变量：下划线前缀 (_hwnd, _client, _d3dDevice)
- 常量：k前缀 (kDefaultWidth, kMaxInstances)
- 命名空间：lowercase (namespace cefview)
- **Status**: PASS - 遵循现有代码规范

### ✅ Principle VI: Header Guards & File Organization
- 使用#ifndef/#define/#endif + #pragma once双重保护
- Doxygen格式文档头（@file, @brief, @version, @author, @date）
- 包含顺序：相关头文件 → C系统 → C++标准 → CEF → D3D → 项目头文件
- **Status**: PASS - 遵循现有文件组织模式

### ✅ Principle VII: Code Quality & Safety
- 使用std::shared_ptr、std::unique_ptr管理对象生命周期
- 使用CefRefPtr管理CEF对象
- 使用Microsoft::WRL::ComPtr管理D3D11 COM对象
- 所有变量初始化，nullptr替代NULL
- 使用= delete禁用拷贝构造（CefWebView不可复制）
- **Status**: PASS - 严格RAII和智能指针管理

### 🔍 Additional Check: Direct3D 11 Integration
虽然constitution未明确提及D3D11，但作为Windows平台图形API，符合"Windows原生API"原则。
- D3D11属于Windows SDK的一部分
- 不是第三方框架，而是微软官方API
- 用于硬件加速渲染，符合性能目标
- **Status**: PASS - 合理使用平台图形API

**Overall Constitution Compliance**: ✅ PASS - 所有7项核心原则均满足

## Project Structure

### Documentation (this feature)

```text
specs/001-cefwebview-control/
├── plan.md              # 本文件 (实施计划)
├── spec.md              # 功能规范
├── research.md          # Phase 0 研究文档
├── data-model.md        # Phase 1 数据模型
├── quickstart.md        # Phase 1 快速开始指南
├── contracts/           # Phase 1 API契约
│   └── cefwebview-api.md
└── checklists/          # 质量检查清单
    └── requirements.md
```

### Source Code (repository root)

```text
src/
├── app/                           # 应用层（演示程序）
│   ├── main.cpp                   # 应用入口点，CEF初始化
│   ├── MainWindow.h               # 主窗口类声明（已存在，需完善）
│   ├── MainWindow.cpp             # 主窗口实现（已存在，需完善）
│   └── CMakeLists.txt             # 应用层构建配置
│
├── cefview/                       # CEF集成层（核心库）
│   ├── client/                    # CEF客户端实现（已存在）
│   │   ├── CefViewClient.h        # CEF客户端类（已存在，可能需扩展）
│   │   ├── CefViewClient.cpp      # CEF客户端实现（已存在，可能需扩展）
│   │   ├── CefViewApp.h           # CEF应用类（已存在）
│   │   ├── CefViewApp.cpp         # CEF应用实现（已存在）
│   │   └── ...                    # 其他已有文件
│   │
│   ├── handler/                   # CEF事件处理器（已存在）
│   │   └── ...                    # 已有handler文件
│   │
│   ├── manager/                   # CEF管理器（已存在）
│   │   ├── CefManager.h           # CEF生命周期管理（已存在）
│   │   └── CefManager.cpp         # CEF管理实现（已存在）
│   │
│   ├── osr_renderer/              # 离屏渲染器（已存在，需完善）
│   │   ├── osr_renderer.h         # 离屏渲染接口（已存在）
│   │   ├── osr_renderer.cc        # 离屏渲染实现（已存在）
│   │   ├── D3D11Renderer.h        # D3D11渲染后端（新增）
│   │   └── D3D11Renderer.cpp      # D3D11渲染实现（新增，参考dx11/）
│   │
│   ├── view/                      # 视图组件（控件核心）
│   │   ├── CefWebView.h           # 浏览器控件类（已存在，需完善）
│   │   ├── CefWebView.cpp         # 浏览器控件实现（已存在，需完善）
│   │   ├── CefWebViewSetting.h    # 控件配置类（已存在）
│   │   ├── CefViewClientDelegate.h      # 委托类（已存在，需完善）
│   │   ├── CefViewClientDelegate.cpp    # 委托实现（已存在，需完善）
│   │   └── ...                    # 其他已有文件
│   │
│   ├── utils/                     # 工具类（已存在）
│   │   ├── FileUtil.h/cc          # 文件工具（已存在）
│   │   ├── WinUtil.h/cc           # Windows工具（已存在）
│   │   ├── GeometryUtil.h/cc      # 几何工具（已存在）
│   │   └── Export.h               # 导出宏定义（已存在）
│   │
│   └── CMakeLists.txt             # cefview库构建配置
│
├── sub_process/                   # CEF子进程（已存在）
│   └── main_process.cpp           # 子进程入口（已存在）
│
dx11/ (参考实现，不修改)           # D3D11参考代码
├── DX11RenderBackend.h            # D3D11渲染后端头文件（参考）
└── DX11RenderBackend.cpp          # D3D11渲染实现（参考）

libcef/                            # CEF二进制文件（已存在）
CMakeLists.txt                     # 根构建配置（已存在）
```

**Structure Decision**: 

采用**单项目结构**（Option 1变体），项目已有清晰的三层架构：
1. **app/** - 应用层：演示程序，展示CefWebView控件使用方法
2. **cefview/** - 库层：CEF集成和浏览器控件实现，可作为独立库使用
3. **sub_process/** - 子进程层：CEF多进程架构所需的辅助进程

**关键决策**:
- 保留现有目录结构和文件命名模式
- 新增`D3D11Renderer.h/cpp`到`osr_renderer/`用于离屏渲染
- 参考`dx11/`目录下的实现，但不直接修改或移动这些文件
- 完善已有的`CefWebView`、`MainWindow`等类，而非重写

## Complexity Tracking

> 本节记录任何需要特殊说明的架构复杂度或constitution例外情况

| 考量点 | 说明 | 合理性 |
|--------|------|--------|
| Direct3D 11依赖 | 引入D3D11用于离屏渲染硬件加速 | 合理：D3D11是Windows原生图形API，属于Windows SDK的一部分，不违反"仅使用Windows原生API"原则。相比GDI软件渲染，性能提升显著（硬件加速）。 |
| 多接口继承 | CefViewClient继承多个CEF Handler接口 | 合理：CEF框架设计模式要求，这些接口都是纯虚接口（类似Java interface），不是implementation继承。符合constitution允许的"接口多继承"。 |
| COM对象管理 | 使用Microsoft::WRL::ComPtr管理D3D11对象 | 合理：D3D11对象基于COM模型，ComPtr是微软官方提供的智能指针，符合RAII原则。不使用ComPtr会导致资源泄漏风险。 |
| 参考现有代码 | 离屏渲染实现参考dx11/目录代码 | 合理：复用已验证的D3D11渲染逻辑，降低实现风险。需要调整以符合项目命名和架构规范。 |

**结论**: 无constitution原则违反，所有技术选择均有充分的合理性支撑。
