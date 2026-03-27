# cef_view

基于 CEF (Chromium Embedded Framework) 的桌面端嵌入式浏览器示例项目，支持 Windows / macOS。

- CEF 版本：**142.0.17** (Chromium 142.0.7444.176)
- 构建系统：CMake 3.14+
- Windows 工具链：Visual Studio 2022

---

## 快速开始（脚本方式）

项目提供一键脚本，自动下载 CEF 并生成 Visual Studio 工程。

### 默认模式（Release，最小包）

```bash
# CMD
GenerateProject.bat

# PowerShell
.\GenerateProject.ps1
```

脚本会自动下载 CEF **minimal** 分发包（仅含 Release 二进制和头文件），体积较小，适合日常开发。

### Debug CEF 模式（全量包）

```bash
# CMD
GenerateProject.bat --debug-cef

# PowerShell
.\GenerateProject.ps1 --debug-cef
```

脚本会下载 CEF **standard** 分发包（包含 Debug + Release 完整二进制），并在 CMake 中启用 `CEF_USE_DEBUG=ON`，允许在 VS 中以 Debug 配置调试 CEF 本身。

### 脚本参数

| 参数 | 说明 |
|------|------|
| `--debug-cef` | 下载全量 CEF 包，启用 Debug CEF 二进制支持 |
| `--help` / `-h` | 显示帮助信息 |

### 构建流程

1. 运行脚本后，工程生成在 `build/` 目录
2. 用 Visual Studio 打开 `build\CefView.sln`
3. 选择 Debug 或 Release 配置进行构建
4. 构建产物在 `build\src\app\<Configuration>\` 目录下

---

## 手动方式（自行下载 CEF）

如果你已经有 CEF 分发包，或需要更精细地控制构建参数，可以跳过脚本手动操作。

### 1. 下载 CEF

从 https://cef-builds.spotifycdn.com/index.html 下载对应版本：

- **版本**：`142.0.17` / Chromium `142.0.7444.176`
- **平台**：Windows 64-bit
- **分发类型**：
  - **Minimal Distribution**：仅 Release 二进制 + 头文件，体积小（默认推荐）
  - **Standard Distribution**：包含 Debug + Release 完整二进制，体积较大（需要调试 CEF 时选择）

### 2. 解压到项目根目录

将下载的包解压并重命名为 `libcef/`，使目录结构为：

```
cef_view/
├── libcef/
│   ├── cmake/
│   ├── include/
│   ├── libcef_dll/
│   ├── Release/          # 始终存在
│   ├── Debug/            # 仅 Standard Distribution 包含
│   └── Resources/
├── src/
├── CMakeLists.txt
└── ...
```

### 3. 生成工程

```bash
# 默认模式（Minimal 包 / Release CEF only）
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build

# Debug CEF 模式（Standard 包 / Debug + Release CEF）
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build -DCEF_USE_DEBUG=ON
```

### 4. 构建

```bash
# 在 VS 中打开构建
start build\CefView.sln

# 或命令行构建
cmake --build build --config Debug
cmake --build build --config Release
```

---

## CMake 参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `CEF_USE_DEBUG` | `OFF` | 是否使用 CEF Debug 二进制。`ON` 时需要 Standard Distribution（含 `Debug/` 目录），链接和拷贝会按 VS 配置自动选择 Debug/Release CEF。`OFF` 时使用 Minimal Distribution，所有配置统一使用 Release CEF 二进制 |
| `USE_SANDBOX` | `OFF` | CEF 沙箱开关 |
| `BUILD_WITH_MT` | `ON` | Windows 下使用 `/MT` 静态 CRT（`OFF` 则用 `/MD`） |
| `WEBVIEW_BUILD_STATIC` | `ON` | 将 cefview 库构建为静态库（`OFF` 则为动态库） |

---

## 项目结构

```
cef_view/
├── CMakeLists.txt              # 根 CMake 配置
├── GenerateProject.bat         # Windows CMD 一键构建脚本
├── GenerateProject.ps1         # Windows PowerShell 一键构建脚本
├── libcef/                     # CEF 分发包（脚本自动下载或手动放置）
└── src/
    ├── app/                    # 主应用程序 (cefapp)
    │   ├── win/                # Windows 平台入口和窗口
    │   └── mac/                # macOS 平台入口和窗口
    ├── sub_process/            # CEF 子进程 (browser.exe / Helper bundles)
    ├── cef_view/               # 核心 CEF 封装库 (cefview)
    │   ├── bridge/             # JS Bridge
    │   ├── client/             # CEF Client 委托
    │   ├── global/             # CEF 生命周期管理
    │   ├── osr/                # Off-Screen Rendering
    │   ├── scheme/             # 自定义 Scheme
    │   ├── utils/              # 工具函数
    │   └── view/               # 视图层
    └── resource/               # 应用资源文件
```

## 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `cefview` | 静态库（默认） | 核心 CEF 封装库 |
| `cefapp` | 可执行程序 | 主应用程序 |
| `browser` (Win) / Helper bundles (Mac) | 可执行程序 | CEF 子进程 |
